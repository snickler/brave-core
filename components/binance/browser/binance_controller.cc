/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_controller.h"

#include <algorithm>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/time/time.h"
#include "base/token.h"
#include "brave/common/pref_names.h"
#include "brave/components/binance/browser/binance_json_parser.h"
#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

GURL BinanceController::oauth_endpoint_("https://accounts.binance.com");

namespace {

// To do, Add as env var
const std::string client_secret = "";
const std::string client_id = "Fx5sIiTEI5";
const std::string oauth_url =
    "https://accounts.binance.com/en/oauth/authorize";
const std::string encoded_uri =
    "https%3A%2F%2Fwww.brave.com%2Fbinance%2Foauth%2Fcallback";
const std::string oauth_scope = "user:email,user:address";

const unsigned int kRetriesCountOnNetworkChange = 1;
net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("binance_controller", R"(
      semantics {
        sender: "Binance Controller"
        description:
          "This service is used to communicate with Binance "
          "on behalf of the user interacting with the Binance widget."
        trigger:
          "Triggered by user connecting the Binance widget."
        data:
          "Account balance for the widget."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on the new tab page."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

BinanceController::BinanceController(content::BrowserContext* context)
    : context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context_)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {
  LoadTokensFromPrefs();
}

BinanceController::~BinanceController() {
}

std::string BinanceController::GetOAuthClientUrl () {
  // To do, use a better formatting solution :)
  const std::string client_url =
      oauth_url + "?response_type=code&client_id=" + client_id +
      "&redirect_uri=" + encoded_uri + "&scope=" + oauth_scope;
  return client_url;
}

bool BinanceController::GetAccessToken(
    const std::string& code,
    GetAccessTokenCallback callback) {
  auto internal_callback = base::BindOnce(
      &BinanceController::OnGetAccessToken,
      base::Unretained(this), std::move(callback));

  const std::string code_param = std::string("code=") + code;
  const std::string grant_type = std::string("grant_type=authorization_code");
  const std::string id_param = std::string("client_id=") + client_id;
  const std::string secret_param = std::string("client_secret=") + client_secret;
  const std::string redirect_uri = std::string("redirect_uri=") + encoded_uri;
  // To do, use a better formatting solution :)
  const std::string formatted_params =
      "?code=" + code_param + "&grant_type=authorization_code&client_id=" +
      id_param + "&client_secret=" + secret_param + "&redirect_uri=" +
      redirect_uri;

  return OAuthRequest(oauth_path_access_token,
                    formatted_params,
                    std::move(internal_callback));
}

void BinanceController::OnGetAccessToken(
    GetAccessTokenCallback callback,
    const int status, const std::string& body,
    const std::map<std::string, std::string>& headers) {
  std::string access_token;
  std::string refresh_token;

  if (status >= 200 && status <= 299) {
    BinanceJSONParser::GetTokensFromJSON(body, &access_token, "access_token");
    BinanceJSONParser::GetTokensFromJSON(body, &refresh_token, "refresh_token");
    SetAccessTokens(access_token, refresh_token);
  }

  std::move(callback).Run(IsUnauthorized(status));
}

bool BinanceController::OAuthRequest(const std::string& path,
                                     const std::string& query_params,
                                     URLRequestCallback callback) {
  std::string oauth_url = oauth_endpoint_.spec();
  std::string request_url = oauth_url + path + query_params;
  auto request = std::make_unique<network::ResourceRequest>();

  request->url = GURL(request_url);
  request->load_flags = net::LOAD_DO_NOT_SEND_COOKIES |
                        net::LOAD_DO_NOT_SAVE_COOKIES |
                        net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE;
  request->method = "POST";

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  Profile* profile = Profile::FromBrowserContext(context_);
  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(profile);
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory, base::BindOnce(
          &BinanceController::OnURLLoaderComplete,
          base::Unretained(this), std::move(iter), std::move(callback)));

  return true;
}

void BinanceController::OnURLLoaderComplete(
    SimpleURLLoaderList::iterator iter,
    URLRequestCallback callback,
    const std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;
  std::map<std::string, std::string> headers;
  if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
    response_code = loader->ResponseInfo()->headers->response_code();
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }

  url_loaders_.erase(iter);

  std::move(callback).Run(
      response_code, response_body ? *response_body : "", headers);
}

bool BinanceController::IsUnauthorized(int status) {
  return status == 401;
}

bool BinanceController::SetAccessTokens(const std::string& access_token,
                                        const std::string& refresh_token) {
  access_token_ = access_token;
  refresh_token_ = refresh_token;

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;

  if (!OSCrypt::EncryptString(access_token, &encrypted_access_token)) {
    LOG(ERROR) << "Could not encrypt and save Binance token info";
    return false;
  }
  if (!OSCrypt::EncryptString(refresh_token, &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not encrypt and save Binance token info";
    return false;
  }

  std::string encoded_encrypted_access_token;
  std::string encoded_encrypted_refresh_token;
  base::Base64Encode(encrypted_access_token, &encoded_encrypted_access_token);
  base::Base64Encode(encrypted_refresh_token, &encoded_encrypted_refresh_token);

  Profile* profile = Profile::FromBrowserContext(context_);
  profile->GetPrefs()->SetString(kBinanceAccessToken, encoded_encrypted_access_token);
  profile->GetPrefs()->SetString(kBinanceRefreshToken,
      encoded_encrypted_refresh_token);

  return true;
}

bool BinanceController::LoadTokensFromPrefs() {
  Profile* profile = Profile::FromBrowserContext(context_);
  std::string encoded_encrypted_access_token =
      profile->GetPrefs()->GetString(kBinanceAccessToken);
  std::string encoded_encrypted_refresh_token =
      profile->GetPrefs()->GetString(kBinanceRefreshToken);

  std::string encrypted_access_token;
  std::string encrypted_refresh_token;
  if (!base::Base64Decode(encoded_encrypted_access_token, &encrypted_access_token) ||
      !base::Base64Decode(encoded_encrypted_refresh_token,
                          &encrypted_refresh_token)) {
    LOG(ERROR) << "Could not Base64 decode Binance token info.";
    return false;
  }

  if (!OSCrypt::DecryptString(encrypted_access_token, &access_token_)) {
    LOG(ERROR) << "Could not decrypt and save Binance token info.";
    return false;
  }
  if (!OSCrypt::DecryptString(encrypted_refresh_token, &refresh_token_)) {
    LOG(ERROR) << "Could not decrypt and save Binance token info.";
    return false;
  }
  return true;
}

std::string BinanceController::GetBinanceTLD() {
  Profile* profile = Profile::FromBrowserContext(context_);

  const std::string usTLD = "us";
  const std::string usCode = "US";
  const std::string globalTLD = "com";

  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());
  const int32_t us_id = country_codes::CountryCharsToCountryID(
      usCode.at(0), usCode.at(1));

  return (user_country_id == us_id) ? usTLD : globalTLD;
}

base::SequencedTaskRunner* BinanceController::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
