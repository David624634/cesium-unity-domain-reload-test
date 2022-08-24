#include "UnityAssetAccessor.h"

#include <CesiumAsync/IAssetResponse.h>

#include <DotNet/CesiumForUnity/NativeDownloadHandler.h>
#include <DotNet/System/Action1.h>
#include <DotNet/System/String.h>
#include <DotNet/UnityEngine/Networking/DownloadHandler.h>
#include <DotNet/UnityEngine/Networking/UnityWebRequest.h>
#include <DotNet/UnityEngine/Networking/UnityWebRequestAsyncOperation.h>

using namespace CesiumAsync;
using namespace DotNet;

namespace {

class UnityAssetResponse : public IAssetResponse {
public:
  UnityAssetResponse(
      const UnityEngine::Networking::UnityWebRequest& request,
      const DotNet::CesiumForUnity::NativeDownloadHandler& handler)
      : _statusCode(uint16_t(request.responseCode())),
        _contentType(request.GetResponseHeader(System::String("Content-Type"))
                         .ToStlString()),
        _handler(handler) {
    this->_headers.emplace("Content-Type", this->_contentType);
    // TODO: get all response headers
  }

  virtual uint16_t statusCode() const override { return _statusCode; }

  virtual std::string contentType() const override { return _contentType; }

  virtual const HttpHeaders& headers() const override { return _headers; }

  virtual gsl::span<const std::byte> data() const override {
    return this->_handler.NativeImplementation().getData();
  }

private:
  uint16_t _statusCode;
  std::string _contentType;
  HttpHeaders _headers;
  ::DotNet::CesiumForUnity::NativeDownloadHandler _handler;
};

class UnityAssetRequest : public IAssetRequest {
public:
  UnityAssetRequest(
      const DotNet::UnityEngine::Networking::UnityWebRequest& request,
      const DotNet::CesiumForUnity::NativeDownloadHandler& handler)
      : _method(request.method().ToStlString()),
        _url(request.url().ToStlString()),
        _headers(),
        _response(request, handler) {
    // TODO: get request headers
  }

  virtual const std::string& method() const override { return _method; }

  virtual const std::string& url() const override { return _url; }

  virtual const HttpHeaders& headers() const override { return _headers; }

  virtual const IAssetResponse* response() const override { return &_response; }

private:
  std::string _method;
  std::string _url;
  HttpHeaders _headers;
  UnityAssetResponse _response;
};

} // namespace

namespace CesiumForUnityNative {

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnityAssetAccessor::get(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& url,
    const std::vector<THeader>& headers) {
  // Sadly, Unity requires us to call this from the main thread.
  return asyncSystem.runInMainThread([asyncSystem, url, headers]() {
    UnityEngine::Networking::UnityWebRequest request =
        UnityEngine::Networking::UnityWebRequest::Get(System::String(url));

    DotNet::CesiumForUnity::NativeDownloadHandler handler{};
    request.downloadHandler(handler);

    for (const auto& header : headers) {
      request.SetRequestHeader(
          System::String(header.first),
          System::String(header.second));
    }

    auto promise =
        asyncSystem
            .createPromise<std::shared_ptr<CesiumAsync::IAssetRequest>>();

    auto future = promise.getFuture();

    UnityEngine::Networking::UnityWebRequestAsyncOperation op =
        request.SendWebRequest();
    op.add_completed(System::Action1<UnityEngine::AsyncOperation>(
        [request, promise = std::move(promise), handler = std::move(handler)](
            const UnityEngine::AsyncOperation& operation) mutable {
          if (request.isDone() && request.error() == nullptr) {
            promise.resolve(
                std::make_shared<UnityAssetRequest>(request, handler));
          } else {
            promise.reject(std::runtime_error(
                "Request failed: " + request.error().ToStlString()));
          }
        }));

    return future;
  });
}

CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
UnityAssetAccessor::request(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& verb,
    const std::string& url,
    const std::vector<THeader>& headers,
    const gsl::span<const std::byte>& contentPayload) {
  throw std::exception();
}

void UnityAssetAccessor::tick() noexcept {}

} // namespace CesiumForUnityNative
