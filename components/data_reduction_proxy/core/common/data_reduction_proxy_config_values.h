// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_CONFIG_VALUES_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_CONFIG_VALUES_H_

#include <vector>

class GURL;

namespace net {
class HostPortPair;
class ProxyServer;
}

namespace data_reduction_proxy {

struct DataReductionProxyTypeInfo;

class DataReductionProxyConfigValues {
 public:
  virtual ~DataReductionProxyConfigValues() {}

  // Returns true if the data reduction proxy promo may be shown.
  // This is independent of whether the data reduction proxy is allowed.
  // TODO(bengr): maybe tie to whether proxy is allowed.
  virtual bool promo_allowed() const = 0;

  // Returns true if the data reduction proxy should not actually use the
  // proxy if enabled.
  virtual bool holdback() const = 0;

  // Returns true if the data reduction proxy configuration may be used.
  virtual bool allowed() const = 0;

  // Returns true if the fallback proxy may be used.
  virtual bool fallback_allowed() const = 0;

  // Returns true if the alternative data reduction proxy configuration may be
  // used.
  virtual bool alternative_allowed() const = 0;

  // Returns true if the alternative fallback data reduction proxy
  // configuration may be used.
  virtual bool alternative_fallback_allowed() const = 0;

  // Returns true if the proxy server uses an HTTP tunnel to provide HTTPS
  // proxying.
  virtual bool UsingHTTPTunnel(const net::HostPortPair& proxy_server) const = 0;

  // Returns the HTTP proxy servers to be used. |use_alternative_configuration|
  // is a temporary feature whilst DataReductionProxyParams is still in use.
  virtual const std::vector<net::ProxyServer>& proxies_for_http(
      bool use_alternative_configuration) const = 0;

  // Returns the HTTPS proxy servers to be used. |use_alternative_configuration|
  // is a temporary feature whilst DataReductionProxyParams is still in use.
  virtual const std::vector<net::ProxyServer>& proxies_for_https(
      bool use_alternative_configuration) const = 0;

  // Returns the URL to check to decide if the secure proxy origin should be
  // used.
  virtual const GURL& secure_proxy_check_url() const = 0;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_COMMON_DATA_REDUCTION_PROXY_CONFIG_VALUES_H_
