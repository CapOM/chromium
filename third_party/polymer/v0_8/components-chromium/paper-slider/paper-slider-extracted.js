
  /**
   * Fired when the slider's value changes.
   *
   * @event value-change
   */

  /**
   * Fired when the slider's immediateValue changes.
   *
   * @event immediate-value-change
   */

  /**
   * Fired when the slider's value changes due to user interaction.
   *
   * Changes to the slider's value due to changes in an underlying
   * bound variable will not trigger this event.
   *
   * @event change
   */

  Polymer({
    is: 'paper-slider',

    behaviors: [
      Polymer.IronRangeBehavior,
      Polymer.IronControlState
    ],

    properties: {

      /**
       * If true, the slider thumb snaps to tick marks evenly spaced based
       * on the `step` property value.
       */
      snaps: {
        type: Boolean,
        value: false,
        notify: true
      },

      /**
       * If true, a pin with numeric value label is shown when the slider thumb
       * is pressed.  Use for settings for which users need to know the exact
       * value of the setting.
       */
      pin: {
        type: Boolean,
        value: false,
        notify: true
      },

      /**
       * The number that represents the current secondary progress.
       */
      secondaryProgress: {
        type: Number,
        value: 0,
        notify: true,
        observer: '_secondaryProgressChanged'
      },

      /**
       * If true, an input is shown and user can use it to set the slider value.
       */
      editable: {
        type: Boolean,
        value: false
      },

      /**
       * The immediate value of the slider.  This value is updated while the user
       * is dragging the slider.
       */
      immediateValue: {
        type: Number,
        value: 0,
        readOnly: true
      },

      /**
       * The maximum number of markers
       */
      maxMarkers: {
        type: Number,
        value: 0,
        notify: true,
        observer: '_maxMarkersChanged'
      },

      /**
       * If true, the knob is expanded
       */
      expand: {
        type: Boolean,
        value: false,
        readOnly: true
      },

      /**
       * True when the user is dragging the slider.
       */
      dragging: {
        type: Boolean,
        value: false,
        readOnly: true
      },

      transiting: {
        type: Boolean,
        value: false,
        readOnly: true
      },

      markers: {
        readOnly: true,
        value: []
      },
    },

    observers: [
      '_updateKnob(value, min, max, snaps, step)',
      '_minChanged(min)',
      '_maxChanged(max)',
      '_valueChanged(value)',
      '_immediateValueChanged(immediateValue)'
    ],

    ready: function() {
      // issue polymer/polymer#1305
      this.async(function() {
        this._updateKnob(this.value);
        this._updateInputValue();
      }, 1);
    },

    /**
     * Increases value by `step` but not above `max`.
     * @method increment
     */
    increment: function() {
      this.value = this._clampValue(this.value + this.step);
    },

    /**
     * Decreases value by `step` but not below `min`.
     * @method decrement
     */
    decrement: function() {
      this.value = this._clampValue(this.value - this.step);
    },

    _updateKnob: function(value) {
      this._positionKnob(this._calcRatio(value));
    },

    _minChanged: function() {
      this.setAttribute('aria-valuemin', this.min);
    },

    _maxChanged: function() {
      this.setAttribute('aria-valuemax', this.max);
    },

    _valueChanged: function() {
      this.setAttribute('aria-valuenow', this.value);
      this.fire('value-change');
    },

    _immediateValueChanged: function() {
      if (!this.dragging) {
        this.value = this.immediateValue;
      }
      this._updateInputValue();
      this.fire('immediate-value-change');
    },

    _secondaryProgressChanged: function() {
      this.secondaryProgress = this._clampValue(this.secondaryProgress);
    },

    _updateInputValue: function() {
      if (this.editable) {
        this.$$('#input').value = this.immediateValue;
      }
    },

    _expandKnob: function() {
      this._setExpand(true);
    },

    _resetKnob: function() {
      this._expandJob && this._expandJob.stop();
      this._setExpand(false);
    },

    _positionKnob: function(ratio) {
      this._setImmediateValue(this._calcStep(this._calcKnobPosition(ratio)) || 0);
      this._setRatio(this.snaps ? this._calcRatio(this.immediateValue) : ratio);
      this.$.sliderKnob.style.left = this.ratio * 100 + '%';
    },

    _inputChange: function() {
      this.value = this.$$('#input').value;
      this.fire('change');
    },

    _calcKnobPosition: function(ratio) {
      return (this.max - this.min) * ratio + this.min;
    },

    _onTrack: function(e) {
      switch (event.detail.state) {
        case 'end':
          this._trackEnd(event);
          break;
        case 'track':
          this._trackX(event);
          break;
        case 'start':
          this._trackStart(event);
          break;
      }
    },

    _trackStart: function(e) {
      this._w = this.$.sliderBar.offsetWidth;
      this._x = this.ratio * this._w;
      this._startx = this._x || 0;
      this._minx = - this._startx;
      this._maxx = this._w - this._startx;
      this.$.sliderKnob.classList.add('dragging');
      this._setDragging(true);
      e.preventDefault();
    },

    _trackX: function(e) {
      if (!this.dragging) {
        this._trackStart(e);
      }
      var x = Math.min(this._maxx, Math.max(this._minx, e.detail.dx));
      this._x = this._startx + x;
      this._setImmediateValue(this._calcStep(
          this._calcKnobPosition(this._x / this._w)) || 0);
      var s =  this.$.sliderKnob.style;
      s.transform = s.webkitTransform = 'translate3d(' + (this.snaps ?
          (this._calcRatio(this.immediateValue) * this._w) - this._startx : x) + 'px, 0, 0)';
    },

    _trackEnd: function() {
      var s =  this.$.sliderKnob.style;
      s.transform = s.webkitTransform = '';
      this.$.sliderKnob.classList.remove('dragging');
      this._setDragging(false);
      this._resetKnob();
      this.value = this.immediateValue;
      this.fire('change');
    },

    _knobdown: function(e) {
      e.preventDefault();
      this._expandKnob();
    },

    _bardown: function(e) {
      e.preventDefault();
      this._setTransiting(true);
      this._w = this.$.sliderBar.offsetWidth;
      var rect = this.$.sliderBar.getBoundingClientRect();
      var ratio = (e.detail.x - rect.left) / this._w;
      this._positionKnob(ratio);
      this._expandJob = this.debounce(this._expandJob, this._expandKnob, 60);

      this.async(function() {
        this.fire('change');
      });
    },

    _knobTransitionEnd: function(e) {
      if (e.target === this.$.sliderKnob) {
        this._setTransiting(false);
      }
    },

    _maxMarkersChanged: function(maxMarkers) {
      var l = (this.max - this.min) / this.step;
      if (!this.snaps && l > maxMarkers) {
        this._setMarkers([]);
      } else {
        this._setMarkers(new Array(l));
      }
    },

    _getClassNames: function() {
      var classes = {};

      classes['disabled'] = this.disabled;
      classes['pin'] = this.pin;
      classes['snaps'] = this.snaps;
      classes['ring'] = this.immediateValue <= this.min;
      classes['expand'] = this.expand;
      classes['dragging'] = this.dragging;
      classes['transiting'] = this.transiting;
      classes['editable'] = this.editable;

      return Object.keys(classes).filter(
        function(className) {
          return classes[className];
        }).join(' ');
    },

    _incrementKey: function(ev, keys) {
      if (keys.key === 'end') {
        this.value = this.max;
      } else {
        this.increment();
      }
      this.fire('change');
    },

    _decrementKey: function(ev, keys) {
      if (keys.key === 'home') {
        this.value = this.min;
      } else {
        this.decrement();
      }
      this.fire('change');
    }
  })
