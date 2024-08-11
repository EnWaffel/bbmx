# bbmx Lua API

> All bbmx API functions are prefixed with 'bbmx'.  
>\> All colors and brightness range from 0 to 255!

## User-defined Functions

Some functions need to be defined by the user in their script. These functions are used for things like setup and an update loop.

___

```lua
function BBMX_setup()
```

This function is the entry point of a bbmx script.  
Setup fixtures, groups and other stuff here.  

```lua
function BBMX_start()
```

This function is called after everything has been set up.

```lua
function BBMX_loop(?delta: number)
```

**OPTIONAL**  
This function is called every update.  
**!** When using this function you need to manually call `bbmx_exit` to exit the script **!**  
*For more info see the `-u` argument in the `CLI.md`.*

```lua
function BBMX_exit()
```

**OPTIONAL**  
This function is called before the script exits.

## Functions

```lua
function bbmx_port(port: string)
```

Sets the COM-Port to connect to.  
`port`: Any COM-Port where the controller is connected to.

```lua
function bbmx_using(model: string)
```

Sets the model to use for the next `bbmx_fixture` call.  
`model`: Any loaded model name.

```lua
function bbmx_opt(option: string, value: any)
```

Sets the option `option` to `value`.  
`option`: The name of the option.  
`value`: The value.

Available Options:

- **universe** (integer value: 1-?)
- **channel-mode** (integer value)

```lua
function bbmx_fixture(fx: string, ?startingAddress: integer)
```

Registers a fixture with the name `fx` and optionally an starting address `startingAddress`.  
When no starting address is given, one will be generated based on the channels of the current selected model.  
`fx`: The name of the fixture.  
`startingAddress`: Optional starting address.  

```lua
function bbmx_fx_reset(fx: string)
```

Resets the fixture `fx`.  
`fx`: The name of the fixture.  

```lua
function bbmx_fx_r(fx: string, value: integer)
```

Sets the color **RED** for the fixture `fx`.  
`fx`: The name of the fixture.  
`value`: The intensity of the color. (0-255)  

```lua
function bbmx_fx_g(fx: string, value: integer)
```

Sets the color **GREEN** for the fixture `fx`.  
`fx`: The name of the fixture.  
`value`: The intensity of the color. (0-255)  

```lua
function bbmx_fx_b(fx: string, value: integer)
```

Sets the color **BLUE** for the fixture `fx`.  
`fx`: The name of the fixture.  
`value`: The intensity of the color. (0-255)  

```lua
function bbmx_fx_w(fx: string, value: integer)
```

Sets the color **WHITE** for the fixture `fx`.  
`fx`: The name of the fixture.  
`value`: The intensity of the color. (0-255)  

___

> Set multiple colors at once!

```lua
function bbmx_fx_rgb(fx: string, red: integer, green: integer, blue: integer)
function bbmx_fx_rgbw(fx: string, red: integer, green: integer, blue: integer, white: integer)
```

___

```lua
function bbmx_fx_tilt(fx: string, value: number, speed: number)
```

Tilts the fixture `fx` to `value` degrees at `speed`.  
`fx`: The name of the fixture.  
`value`: The degree to tilt to.  
`speed`: The speed of tilting. (0-1)  

```lua
function bbmx_fx_pan(fx: string, value: number, speed: number)
```

Pans the fixture `fx` to `value` degrees at `speed`.  
`fx`: The name of the fixture.  
`value`: The degree to pan to.  
`speed`: The speed of panning. (0-1)  

```lua
function bbmx_exit()
```

Exits the script.

```lua
function bbmx_fx_brgt(fx: string, value: integer)
```

Sets the brightness of the fixture `fx`.  
`fx`: The name of the fixture.  
`value`: The intensity.

```lua
function bbmx_write(fx: string, channel: integer, value: integer)
```

Writes `value` to the channel `channel` to fixture `fx`.  
`fx`: The name of the fixture.  
`channel`: The channel to write to.  
`value`: The value to write.

## Timed functions

Timed functions are useful for creating sequences of e.g. movement, etc...  

An example would looke like this:  

```lua
function t1()
  bbmx_fx_rgb("fx1", 255, 0, 0) -- Set the light to show red at full intensity
end

function t2()
  bbmx_fx_rgb("fx1", 0, 255, 0) -- Set the light to show green at full intensity
end

function t3()
  bbmx_fx_rgb("fx1", 0, 0, 255) -- Set the light to show blue at full intensity
end

function t4()
  bbmx_reset_timer() -- Reset the timer to loop forever
end

function BBMX_setup()
  bbmx_port("your port") -- Set the port for the controller

  bbmx_using("your model") -- Select the model

  bbmx_fixture("fx1") -- Register a fixture named 'fx1'

  bbmx_timed("t1", 0) -- Register a timed function to be called on millisecond 0
  bbmx_timed("t2", 500) -- Register a timed function to be called on millisecond 500
  bbmx_timed("t3", 1000) -- Register a timed function to be called on millisecond 1000
  bbmx_timed("t4", 1500) -- Register a timed function to be called on millisecond 1500
end

function BBMX_start()
  bbmx_fx_brgt("fx1", 50) -- Set the brightness to 50
end
```

___

```lua
function bbmx_timed(func: string, time: integer)
```

Registers a function to be timed.  
`func`: The name of the function to be registered. (e.g. t1) **IT MUST BE GLOBAL**  
`time`: The time offset since start of execution for when the function should be called. (in milliseconds)

```lua
function bbmx_reset_timer()
```

Sets the time back to 0.

## Utility functions

```lua
function lerp(a: number, b: number, f: number)
```

Linearly interpolates between `a` and `b` using factor `f`.

## Variables

**Globals:**

```lua
time -- The time in ms since the start of the script.
```
