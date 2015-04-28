

  /**
   * Stamps the template iff the `if` property is truthy.
   *
   * When `if` becomes falsey, the stamped content is hidden but not
   * removed from dom. When `if` subsequently becomes truthy again, the content
   * is simply re-shown. This approach is used due to its favorable performance
   * characteristics: the expense of creating template content is paid only 
   * once and lazily.
   *
   * Set the `restamp` property to true to force the stamped content to be
   * created / destroyed when the `if` condition changes.
   */
  Polymer({

    is: 'x-if',
    extends: 'template',

    properties: {

      'if': {
        type: Boolean,
        value: false
      },

      restamp: {
        type: Boolean,
        value: false
      }

    },

    behaviors: [
      Polymer.Templatizer
    ],

    observers: [
      'render(if, restamp)'
    ],

    render: function() {
      this.debounce('render', function() {
        if (this.if) {
          if (!this.ctor) {
            this._wrapTextNodes(this._content);
            this.templatize(this);
          }
          this._ensureInstance();
        } else if (this.restamp) {
          this._teardownInstance();
        }
        if (!this.restamp && this._instance) {
          this._showHideInstance(this.if);
        }
      });
    },

    _ensureInstance: function() {
      if (!this._instance) {
        // TODO(sorvell): pickup stamping logic from x-repeat
        this._instance = this.stamp();
        var root = this._instance.root;
        this._instance._children = Array.prototype.slice.call(root.childNodes);
        // TODO(sorvell): this incantation needs to be simpler.
        var parent = Polymer.dom(Polymer.dom(this).parentNode);
        parent.insertBefore(root, this);
      }
    },

    _teardownInstance: function() {
      if (this._instance) {
        var parent = Polymer.dom(Polymer.dom(this).parentNode);
        this._instance._children.forEach(function(n) {
          parent.removeChild(n);
        });
        this._instance = null;
      }
    },

    _wrapTextNodes: function(root) {
      // wrap text nodes in span so they can be hidden.
      for (var n = root.firstChild; n; n=n.nextSibling) {
        if (n.nodeType === Node.TEXT_NODE) {
          var s = document.createElement('span');
          root.insertBefore(s, n);
          s.appendChild(n);
          n = s;
        }
      }
    },

    // Implements extension point from Templatizer mixin
    _getStampedChildren: function() {
      return this._instance._children;
    },

    _showHideInstance: function(showing) {
      this._getAllStampedChildren().forEach(function(n) {
        if (n.setAttribute) {
          this.serializeValueToAttribute(!showing, 'hidden', n);
        }
      }, this);
    },

    // Implements extension point from Templatizer mixin
    // Called as side-effect of a host property change, responsible for
    // notifying parent.<prop> path change on instance
    _forwardParentProp: function(prop, value) {
      if (this._instance) {
        this._instance.parent[prop] = value;
        this._instance.notifyPath('parent.' + prop, value, true);
      }
    },

    // Implements extension point from Templatizer
    // Called as side-effect of a host path change, responsible for
    // notifying parent.<path> path change on each row
    _forwardParentPath: function(path, value) {
      if (this._instance) {
        this._instance.notifyPath('parent.' + path, value, true);
      }
    }

  });

