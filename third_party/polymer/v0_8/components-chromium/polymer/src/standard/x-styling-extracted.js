
  (function() {

    var baseAttachedCallback = Polymer.Base.attachedCallback;
    var baseSerializeValueToAttribute = Polymer.Base.serializeValueToAttribute;

    var nativeShadow = Polymer.Settings.useNativeShadow;

    // TODO(sorvell): consider if calculating properties and applying
    // styles with properties should be separate modules.
    Polymer.Base._addFeature({

      attachedCallback: function() {
        baseAttachedCallback.call(this);
        if (!this._xScopeSelector) {
          this._updateOwnStyles();
        }
      },

      _updateOwnStyles: function() {
        if (this.enableCustomStyleProperties) {
          this._styleProperties = this._computeStyleProperties();
          this._applyStyleProperties(this._styleProperties);
        }
      },

      _computeStyleProperties: function() {
        var props = {};
        this.simpleMixin(props, this._computeStylePropertiesFromHost());
        this.simpleMixin(props, this._computeOwnStyleProperties());
        this._reifyCustomProperties(props);
        return props;
      },

      _computeStylePropertiesFromHost: function() {
        // TODO(sorvell): experimental feature, global defaults!
        var props = {}, styles = [Polymer.StyleDefaults.defaultSheet];
        var host = this.domHost;
        if (host) {
          // enable finding styles in hosts without `enableStyleCustomProperties`
          if (!host._styleProperties) {
            host._styleProperties = host._computeStyleProperties();
          }
          props = Object.create(host._styleProperties);
          styles = host._styles;
        }
        this.simpleMixin(props,
          this._customPropertiesFromStyles(styles, host));
        return props;

      },

      _computeOwnStyleProperties: function() {
        var props = {};
        this.simpleMixin(props, this._customPropertiesFromStyles(this._styles));
        if (this.styleProperties) {
          for (var i in this.styleProperties) {
            props[i] = this.styleProperties[i];
          }
        }
        return props;
      },

      _customPropertiesFromStyles: function(styles, hostNode) {
        var props = {};
        var p = this._customPropertiesFromRule.bind(this, props, hostNode);
        if (styles) {
          for (var i=0, l=styles.length, s; (i<l) && (s=styles[i]); i++) {
            Polymer.StyleUtil.forEachStyleRule(this._rulesForStyle(s), p);
          }
        }
        return props;
      },

      // test if a rule matches the given node and if so,
      // collect any custom properties
      // TODO(sorvell): support custom variable assignment within mixins
      _customPropertiesFromRule: function(props, hostNode, rule) {
        hostNode = hostNode || this;
        // TODO(sorvell): file crbug, ':host' does not match element.
        if (this.elementMatches(rule.selector) ||
          ((hostNode === this) && (rule.selector === ':host'))) {
          // --g: var(--b); or --g: 5;
          this._collectPropertiesFromRule(rule, CUSTOM_VAR_ASSIGN, props);
          // --g: { ... }
          this._collectPropertiesFromRule(rule, CUSTOM_MIXIN_ASSIGN, props);
        }
      },

      // given a rule and rx that matches key and value, set key in properties
      // to value
      _collectPropertiesFromRule: function(rule, rx, properties) {
        var m;
        while (m = rx.exec(rule.cssText)) {
          properties[m[1]] = m[2].trim();
        }
      },

      _reifyCustomProperties: function(props) {
        for (var i in props) {
          props[i] = this._valueForCustomProperty(props[i], props);
        }
      },

      _valueForCustomProperty: function(property, props) {
        var cv;
        while ((typeof property === 'string') &&
          (cv = property.match(CUSTOM_VAR_VALUE))) {
          property = props[cv[1]];
        }
        return property;
      },

      // apply styles
      _applyStyleProperties: function(bag) {
        var s$ = this._styles;
        if (s$) {
          var style = styleFromCache(this.is, bag, s$);
          var old = this._xScopeSelector;
          this._ensureScopeSelector(style ? style._scope : null);
          if (!style) {
            var cssText = this._generateCustomStyleCss(bag, s$);
            style = cssText ? this._applyCustomCss(cssText) : {};
            cacheStyle(this.is, style, this._xScopeSelector,
              this._styleProperties, s$);
          } else if (nativeShadow) {
            this._applyCustomCss(style.textContent);
          }
          if (style.textContent || old /*&& !nativeShadow*/) {
            this._applyXScopeSelector(this._xScopeSelector, old);
          }
        }
      },

      _applyXScopeSelector: function(selector, old) {
        var c = this._scopeCssViaAttr ? this.getAttribute(SCOPE_NAME) :
          this.className;
        v = old ? c.replace(old, selector) :
          (c ? c + ' ' : '') + XSCOPE_NAME + ' ' + selector;
        if (c !== v) {
          if (this._scopeCssViaAttr) {
            this.setAttribute(SCOPE_NAME, v);
          } else {
            this.className = v;
          }
        }
      },

      _generateCustomStyleCss: function(properties, styles) {
        var b = this._applyPropertiesToRule.bind(this, properties);
        var cssText = '';
        // TODO(sorvell): don't redo parsing work each time as below;
        // instead create a sheet with just custom properties
        for (var i=0, l=styles.length, s; (i<l) && (s=styles[i]); i++) {
          cssText += this._transformCss(s.textContent, b) + '\n\n';
        }
        return cssText.trim();
      },

      _transformCss: function(cssText, callback) {
        return nativeShadow ?
          Polymer.StyleUtil.toCssText(cssText, callback) :
          Polymer.StyleTransformer.css(cssText, this.is, this.extends, callback,
            this._scopeCssViaAttr);
      },

      _xScopeCount: 0,

      _ensureScopeSelector: function(selector) {
        selector = selector || (this.is + '-' +
          (Object.getPrototypeOf(this)._xScopeCount++));
        this._xScopeSelector = selector;
      },

      _applyCustomCss: function(cssText) {
        if (this._customStyle) {
          this._customStyle.textContent = cssText;
        } else if (cssText) {
          this._customStyle = Polymer.StyleUtil.applyCss(cssText,
            this._xScopeSelector,
            nativeShadow ? this.root : null);
        }
        return this._customStyle;
      },

      _applyPropertiesToRule: function(properties, rule) {
        if (!nativeShadow) {
          this._scopifyRule(rule);
        }
        if (rule.cssText.match(CUSTOM_RULE_RX)) {
          rule.cssText = this._applyPropertiesToText(rule.cssText, properties);
        } else {
          rule.cssText = '';
        }
        //console.log(rule.cssText);
      },

      _applyPropertiesToText: function(cssText, props) {
        var output = '';
        var m, v;
        // e.g. color: var(--color);
        while (m = CUSTOM_VAR_USE.exec(cssText)) {
          v = props[m[2]];
          if (v) {
            output += '\t' + m[1].trim() + ': ' + this._propertyToCss(v);
          }
        }
        // e.g. @mixin(--stuff);
        while (m = CUSTOM_MIXIN_USE.exec(cssText)) {
          v = m[1];
          if (v) {
            var parts = v.split(' ');
            for (var i=0, p; i < parts.length; i++) {
              p = props[parts[i].trim()];
              if (p) {
                output += '\t' + this._propertyToCss(p);
              }
            }
          }
        }
        return output;
      },

      _propertyToCss: function(property) {
        var p = property.trim();
        p = p[p.length-1] === ';' ? p : p + ';';
        return p + '\n';
      },

      // Strategy: x scope shim a selector e.g. to scope `.x-foo-42` (via classes):
      // non-host selector: .a.x-foo -> .x-foo-42 .a.x-foo
      // host selector: x-foo.wide -> x-foo.x-foo-42.wide
      _scopifyRule: function(rule) {
        var selector = rule.selector;
        var host = this.is;
        var rx = new RegExp(HOST_SELECTOR_PREFIX + host + HOST_SELECTOR_SUFFIX);
        var parts = selector.split(',');
        var scope = this._scopeCssViaAttr ?
          SCOPE_PREFIX + this._xScopeSelector + SCOPE_SUFFIX :
          '.' + this._xScopeSelector;
        for (var i=0, l=parts.length, p; (i<l) && (p=parts[i]); i++) {
          parts[i] = p.match(rx) ?
            p.replace(host, host + scope) :
            scope + ' ' + p;
        }
        rule.selector = parts.join(',');
      },

      _scopeElementClass: function(element, selector) {
        if (!nativeShadow && !this._scopeCssViaAttr) {
          selector += (selector ? ' ' : '') + SCOPE_NAME + ' ' + this.is +
            (element._xScopeSelector ? ' ' +  XSCOPE_NAME + ' ' +
            element._xScopeSelector : '');
        }
        return selector;
      },

      // override to ensure whenever classes are set, we need to shim them.
      serializeValueToAttribute: function(value, attribute, node) {
        if (attribute === 'class') {
          // host needed to scope styling.
          var host = node === this ?
            Polymer.dom(this).getOwnerRoot() || this.dataHost :
            this;
          if (host) {
            value = host._scopeElementClass(node, value);
          }
        }
        baseSerializeValueToAttribute.call(this, value, attribute, node);
      },

      updateStyles: function() {
        this._updateOwnStyles();
        this._updateRootStyles(this.root);
      },

      updateHostStyles: function() {
        var host = Polymer.dom(this).getOwnerRoot() || this.dataHost;
        if (host) {
          host.updateStyles();
        } else {
          this._updateRootStyles(document);
        }
      },

      _updateRootStyles: function(root) {
        // TODO(sorvell): temporary way to find local dom that needs
        // x-scope styling.
        var scopeSelector = this._scopeCssViaAttr ?
          '[' + SCOPE_NAME + '~=' + XSCOPE_NAME + ']' : '.' + XSCOPE_NAME;
        var c$ = Polymer.dom(root).querySelectorAll(scopeSelector);
        for (var i=0, l= c$.length, c; (i<l) && (c=c$[i]); i++) {
          if (c.updateStyles) {
            c.updateStyles();
          }
        }
      }

    });

    var styleCache = {};
    function cacheStyle(is, style, scope, bag, styles) {
      style._scope = scope;
      style._properties = bag;
      style._styles = styles;
      var s$ = styleCache[is] = styleCache[is] || [];
      s$.push(style);
    }

    function styleFromCache(is, bag, checkStyles) {
      var styles = styleCache[is];
      if (styles) {
        for (var i=0, s; i < styles.length; i++) {
          s = styles[i];
          if (objectsEqual(bag, s._properties) &&
            objectsEqual(checkStyles,  s._styles)) {
            return s;
          }
        }
      }
    }

    function objectsEqual(a, b) {
      for (var i in a) {
        if (a[i] !== b[i]) {
          return false;
        }
      }
      for (var i in b) {
        if (a[i] !== b[i]) {
          return false;
        }
      }
      return true;
    }

    var SCOPE_NAME= Polymer.StyleTransformer.SCOPE_NAME;
    var XSCOPE_NAME = 'x-scope';
    var SCOPE_PREFIX = '[' + SCOPE_NAME + '~=';
    var SCOPE_SUFFIX = ']';
    var HOST_SELECTOR_PREFIX = '(?:^|[^.])';
    var HOST_SELECTOR_SUFFIX = '($|[.:[\\s>+~])';
    var CUSTOM_RULE_RX = /mixin|var/;
    var CUSTOM_VAR_ASSIGN = /(--[^\:;]*?):\s*?([^;{]*?);/g;
    var CUSTOM_MIXIN_ASSIGN = /(--[^\:;]*?):[^{;]*?{([^}]*?)}/g;
    var CUSTOM_VAR_VALUE = /^var\(([^)]*?)\)/;
    var CUSTOM_VAR_USE = /(?:^|[;}\s])([^;{}]*?):[\s]*?var\(([^)]*)?\)/gim;
    var CUSTOM_MIXIN_USE = /mixin\(([^)]*)\)/gim;

  })();
