#include "utils.hpp"

#include "set_exporter.hpp"
#include "set_lister.hpp"

#include <curl/curl.h>
#include <filesystem>
#include <iostream>
#include <string>

auto printHelp() -> void {
  std::cout
      << "MTGJSON Extractor CLI\n"
      << "Actions (Pick one):\n"
      << "  --list                        Shows available MTG expansion sets\n"
      << "  --all                         Exports unique cards from ALL sets "
         "to markdown\n"
      << "  --last                        Exports ONLY the most recently "
         "released set\n"
      << "  --set <CODE>                  Exports unique cards from ONE set "
         "(e.g. ONE, ARN)\n"
      << "Options:\n"
      << "  --setType <TYPE>              Filters parsing to only parse sets "
         "matching TYPE (e.g. expansion, commander, core)\n"
      << "  --outDir <DIR>                Specifies base output directory "
         "(Default: extraction)\n"
      << "  --prune                       Automatically deletes markdown files "
         "if 0 cards were exported\n"
      << "Example:\n"
      << "  mtg_extractor --all --setType expansion --outDir ./my_main_sets\n"
      << "  mtg_extractor --list --setType core\n";
}

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    printHelp();
    return 1;
  }

  std::string action;
  std::string targetSet;
  std::string setType;
  std::string outDir = "extraction";
  bool pruneEmpty = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--list") {
      action = "list";
    } else if (arg == "--all") {
      action = "all";
    } else if (arg == "--last") {
      action = "last";
    } else if (arg == "--prune") {
      pruneEmpty = true;
    } else if (arg == "--set" && i + 1 < argc) {
      action = "set";
      targetSet = argv[++i];
    } else if (arg == "--setType" && i + 1 < argc) {
      setType = argv[++i];
    } else if (arg == "--outDir" && i + 1 < argc) {
      outDir = argv[++i];
    } else {
      std::cerr << "Unknown or incomplete argument: " << arg << "\n";
      printHelp();
      return 1;
    }
  }

  if (action.empty()) {
    std::cerr << "Error: No primary action specified (e.g. --all, --list).\n";
    printHelp();
    return 1;
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);

  if (action == "list") {
    listSets(setType);
  } else {
    std::string finalDir = outDir + "/" + getTimestampString();
    std::filesystem::create_directories(finalDir);

    if (action == "set") {
      exportSet(targetSet, finalDir, pruneEmpty);
    } else if (action == "all") {
      exportAllSets(finalDir, setType, pruneEmpty);
    } else if (action == "last") {
      exportLastSet(finalDir, setType, pruneEmpty);
    }
  }

  curl_global_cleanup();
  return 0;
}
