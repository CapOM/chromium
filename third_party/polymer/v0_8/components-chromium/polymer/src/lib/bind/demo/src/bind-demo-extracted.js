

  using(['bind'], function(Bind) {

    var out = document.querySelector('#bd');
    out.innerHTML += '<hr><h3>bind demo</h3><hr>';
    
    // phase one: prototyping

    var model = {};

    Bind.prepareModel(model);

    // 'observer' effects are called if foo changes value as fx(foo, old)

    Bind.addPropertyEffect(model, 'foo', 'observer', 'fooChange');
    Bind.addPropertyEffect(model, 'foo', 'observer', 'fooWork');

    model.fooChange = function(foo) {
      out.innerHTML += '<b>fooChange</b>: effect of changing foo to ' + foo + '\n';
      console.log('fooChange: effect of changing foo to %d', foo);
    };

    model.fooWork = function(foo) {
      out.innerHTML += '<b>fooWork</b>: effect of changing foo to ' + foo + '\n';
      console.log('fooWork: effect of changing foo to %d', foo);
    };

    // 'compute' effect sets the value of bar to the result of computeBar when
    // foo changes value

    /*
    Bind.addPropertyEffect(model, 'foo', 'compute', {
      method: 'computeBar',
      property: 'bar'
    });
    */
    Bind.addComputedPropertyEffect(model, 'bar', 'computeFooTimes2(foo)');

    model.computeFooTimes2 = function(foo) {
      var foo2 = foo * 2;
      out.innerHTML += '<b>computeFooTimes2</b>: calculated ' + foo2 + ' as an effect of changing foo to ' + foo + '\n';
      console.log('computeFooTimes2: calculated %d as effect of changing foo to %d', foo2, foo);
      return foo2;
    };

    // custom effect

    Bind.addBuilder('async', function(model, property, effect) {
      var fn = function() {
        var flag = '_propertyTask';
        clearTimeout(this[flag]);
        this[flag] = setTimeout(function() {
          this.effect(this.property);
          this[flag] = 0;
        }.bind(this));
      };
      var code = fn.toString().split('\n').slice(1, -1).join('\n');
      return code.replace(/property/g, property).replace(/effect/g, effect);
    });

    Bind.addPropertyEffect(model, 'foo', 'async', 'asyncFoo');

    model.asyncFoo = function(foo) {
      out.innerHTML += '<b>asyncFoo</b>: effect of changing foo to ' + foo + '\n';
      console.log('asyncFoo: effect of changing foo to %d', foo);
    };

    // phase two: instancing

    Bind.prepareInstance(model);
    Bind.createBindings(model);

    model.foo = 3;
    model.foo = 6;

  });

