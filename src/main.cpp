#include "utils.hpp"
#include "zip_utils.hpp"

#include "set_exporter.hpp"
#include "set_lister.hpp"

#include <curl/curl.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
      << "  --sets <CODE1,CODE2,...>      Exports unique cards from a "
         "comma-separated list of sets\n"
      << "Options:\n"
      << "  --setType <TYPE>              Filters parsing to only parse sets "
         "matching TYPE (e.g. expansion, commander, core)\n"
      << "  --outDir <DIR>                Specifies base output directory "
         "(Default: extraction)\n"
      << "  --fromDate <YYYY-MM-DD>       Exports only sets released on or "
         "after this date (use with --all)\n"
      << "  --toDate <YYYY-MM-DD>         Exports only sets released on or "
         "before this date (use with --all)\n"
      << "  --fromSet <CODE>              Exports only sets released on or "
         "after CODE's release date (use with --all)\n"
      << "  --toSet <CODE>                Exports only sets released on or "
         "before CODE's release date (use with --all)\n"
      << "  --prune                       Automatically deletes markdown files "
         "if 0 cards were exported\n"
      << "  --zip                         Compresses the output folder into a "
         ".zip archive after export\n"
      << "Examples:\n"
      << "  mtg_extractor --all --setType expansion --outDir ./my_main_sets\n"
      << "  mtg_extractor --list --setType core\n"
      << "  mtg_extractor --sets ONE,ARN,LEA\n"
      << "  mtg_extractor --all --fromDate 2020-01-01 --toDate 2023-12-31\n"
      << "  mtg_extractor --all --fromSet ARN --toSet MIR --zip\n";
}

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    printHelp();
    return 1;
  }

  std::string action;
  std::string targetSet;
  std::vector<std::string> targetSets;
  std::string setType;
  std::string outDir = "extraction";
  std::string fromDate;
  std::string toDate;
  std::string fromSet;
  std::string toSet;
  bool pruneEmpty = false;
  bool doZip = false;

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
    } else if (arg == "--zip") {
      doZip = true;
    } else if (arg == "--set" && i + 1 < argc) {
      action = "set";
      targetSet = argv[++i];
    } else if (arg == "--sets" && i + 1 < argc) {
      action = "sets";
      targetSets = parseCommaSeparated(argv[++i]);
    } else if (arg == "--setType" && i + 1 < argc) {
      setType = argv[++i];
    } else if (arg == "--outDir" && i + 1 < argc) {
      outDir = argv[++i];
    } else if (arg == "--fromDate" && i + 1 < argc) {
      fromDate = argv[++i];
    } else if (arg == "--toDate" && i + 1 < argc) {
      toDate = argv[++i];
    } else if (arg == "--fromSet" && i + 1 < argc) {
      fromSet = argv[++i];
    } else if (arg == "--toSet" && i + 1 < argc) {
      toSet = argv[++i];
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
    } else if (action == "sets") {
      if (targetSets.empty()) {
        std::cerr << "Error: --sets requires at least one set code.\n";
        curl_global_cleanup();
        return 1;
      }
      exportSets(targetSets, finalDir, pruneEmpty);
    } else if (action == "all") {
      exportAllSets(finalDir, setType, pruneEmpty, fromDate, toDate, fromSet,
                    toSet);
    } else if (action == "last") {
      exportLastSet(finalDir, setType, pruneEmpty);
    }

    if (doZip) {
      std::string zipPath = finalDir + ".zip";
      std::cout << "Compressing output to " << zipPath << "...\n";
      if (createZipFromDirectory(finalDir, zipPath))
        std::cout << "Compressed output saved to " << zipPath << "\n";
      else
        std::cerr << "Compression failed.\n";
    }
  }

  curl_global_cleanup();
  return 0;
}
