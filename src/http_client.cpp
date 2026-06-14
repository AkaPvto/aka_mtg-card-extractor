#include "http_client.hpp"

#include <curl/curl.h>
#include <iostream>

// libcurl calls this repeatedly as response data arrives, appending each chunk
// into the string pointed to by userp.
static auto WriteCallback(void *contents, size_t size, size_t nmemb,
                          void *userp) -> size_t {
  auto *buffer = static_cast<std::string *>(userp);
  buffer->append(static_cast<char *>(contents), size * nmemb);
  return size * nmemb;
}

auto fetchURL(const std::string &url) -> std::string {
  CURL *curl;
  CURLcode res;
  std::string readBuffer;

  curl = curl_easy_init();
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
                << "\n";
    }
    curl_easy_cleanup(curl);
  }
  return readBuffer;
}
