

  Polymer({

    is: 'iron-ajax',

    /**
     * Fired when a request is sent.
     *
     * @event request
     */

    /**
     * Fired when a response is received.
     *
     * @event response
     */

    /**
     * Fired when an error is received.
     *
     * @event error
     */

    properties: {
      /**
       * The URL target of the request.
       */
      url: {
        type: String,
        value: ''
      },

      /**
       * An object that contains query parameters to be appended to the
       * specified `url` when generating a request.
       */
      params: {
        type: Object,
        value: function() {
          return {};
        }
      },

      /**
       * The HTTP method to use such as 'GET', 'POST', 'PUT', or 'DELETE'.
       * Default is 'GET'.
       */
      method: {
        type: String,
        value: 'GET'
      },

      /**
       * HTTP request headers to send.
       *
       * Example:
       *
       *     <iron-ajax
       *         auto
       *         url="http://somesite.com"
       *         headers='{"X-Requested-With": "XMLHttpRequest"}'
       *         handle-as="json"
       *         last-response-changed="{{handleResponse}}"></iron-ajax>
       */
      headers: {
        type: Object,
        value: function() {
          return {};
        }
      },

      /**
       * Content type to use when sending data. If the contenttype is set
       * and a `Content-Type` header is specified in the `headers` attribute,
       * the `headers` attribute value will take precedence.
       */
      contentType: {
        type: String,
        value: 'application/x-www-form-urlencoded'
      },

      /**
       * Optional raw body content to send when method === "POST".
       *
       * Example:
       *
       *     <iron-ajax method="POST" auto url="http://somesite.com"
       *         body='{"foo":1, "bar":2}'>
       *     </iron-ajax>
       */
      body: {
        type: String,
        value: ''
      },

      /**
       * Toggle whether XHR is synchronous or asynchronous. Don't change this
       * to true unless You Know What You Are Doing™.
       */
      sync: {
        type: Boolean,
        value: false
      },

      /**
       * Specifies what data to store in the `response` property, and
       * to deliver as `event.response` in `response` events.
       *
       * One of:
       *
       *    `text`: uses `XHR.responseText`.
       *
       *    `xml`: uses `XHR.responseXML`.
       *
       *    `json`: uses `XHR.responseText` parsed as JSON.
       *
       *    `arraybuffer`: uses `XHR.response`.
       *
       *    `blob`: uses `XHR.response`.
       *
       *    `document`: uses `XHR.response`.
       */
      handleAs: {
        type: String,
        value: 'json'
      },

      /**
       * Set the withCredentials flag on the request.
       */
      withCredentials: {
        type: Boolean,
        value: false
      },

      /**
       * If true, automatically performs an Ajax request when either `url` or
       * `params` changes.
       */
      auto: {
        type: Boolean,
        value: false
      },

      /**
       * If true, error messages will automatically be logged to the console.
       */
      verbose: {
        type: Boolean,
        value: false
      },

      /**
       * Will be set to true if there is at least one in-flight request
       * associated with this iron-ajax element.
       */
      loading: {
        type: Boolean,
        notify: true,
        readOnly: true
      },

      /**
       * Will be set to the most recent request made by this iron-ajax element.
       */
      lastRequest: {
        type: Object,
        notify: true,
        readOnly: true
      },

      /**
       * Will be set to the most recent response received by a request
       * that originated from this iron-ajax element. The type of the response
       * is determined by the value of `handleas` at the time that the request
       * was generated.
       */
      lastResponse: {
        type: Object,
        notify: true,
        readOnly: true
      },

      /**
       * Will be set to the most recent error that resulted from a request
       * that originated from this iron-ajax element.
       */
      lastError: {
        type: Object,
        notify: true,
        readOnly: true
      },

      /**
       * An Array of all in-flight requests originating from this iron-ajax
       * element.
       */
      activeRequests: {
        type: Array,
        notify: true,
        readOnly: true,
        value: function() {
          this._setActiveRequests([]);
        }
      },

      /**
       * Length of time in milliseconds to debounce multiple requests.
       */
      debounceDuration: {
        type: Number,
        value: 0,
        notify: true
      },

      _boundHandleResponse: {
        type: Function,
        value: function() {
          return this.handleResponse.bind(this);
        }
      },

      _boundDiscardRequest: {
        type: Function,
        value: function() {
          return this.discardRequest.bind(this);
        }
      }
    },

    observers: [
      'requestOptionsChanged(url, method, params, headers,' +
        'contentType, body, sync, handleAs, withCredentials, auto)'
    ],

    get queryString () {
      var queryParts = [];
      var param;
      var value;

      for (param in this.params) {
        value = this.params[param];
        param = window.encodeURIComponent(param);

        if (value !== null) {
          param += '=' + window.encodeURIComponent(value);
        }

        queryParts.push(param);
      }

      return queryParts.join('&');
    },

    get requestUrl() {
      var queryString = this.queryString;

      if (queryString) {
        return this.url + '?' + queryString;
      }

      return this.url;
    },

    get requestHeaders() {
      var headers = {
        'content-type': this.contentType
      };
      var header;

      if (this.headers instanceof Object) {
        for (header in this.headers) {
          headers[header] = this.headers[header].toString();
        }
      }

      return headers;
    },

    toRequestOptions: function() {
      return {
        url: this.requestUrl,
        method: this.method,
        headers: this.requestHeaders,
        body: this.body,
        async: !this.sync,
        handleAs: this.handleAs,
        withCredentials: this.withCredentials
      };
    },

    requestOptionsChanged: function() {
      this.debounce('generate-request', function() {
        if (!this.url && this.url !== '') {
          return;
        }

        if (this.auto) {
          this.generateRequest();
        }
      }, this.debounceDuration);
    },

    /**
     * Performs an AJAX request to the specified URL.
     *
     * @method generateRequest
     */
    generateRequest: function() {
      var request = document.createElement('iron-request');
      var requestOptions = this.toRequestOptions();

      this.activeRequests.push(request);

      request.completes.then(
        this._boundHandleResponse
      ).catch(
        this.handleError.bind(this, request)
      ).then(
        this._boundDiscardRequest
      );

      request.send(requestOptions);

      this._setLastRequest(request);

      this.fire('request', {
        request: request,
        options: requestOptions
      });

      return request;
    },

    handleResponse: function(request) {
      this._setLastResponse(request.response);
      this.fire('response', request);
    },

    handleError: function(request, error) {
      if (this.verbose) {
        console.error(error);
      }

      this._setLastError({
        request: request,
        error: error
      });
      this.fire('error', {
        request: request,
        error: error
      });
    },

    discardRequest: function(request) {
      var requestIndex = this.activeRequests.indexOf(request);

      if (requestIndex > 0) {
        this.activeRequests.splice(requestIndex, 1);
      }
    }
  });
