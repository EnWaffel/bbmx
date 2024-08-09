# bbmx Lua API

> All bbmx API functions are prefixed with 'bbmx'.  
>\> Parameters prefixed with '?' are optional.  
>\> ALL colors and brightness range from 0 to 255!

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
function bbmx_fx_tilt(fx: string, value: integer, speed: number)
```

Tilts the fixture `fx` to `value` degrees at `speed`.  
`fx`: The name of the fixture.  
`value`: The degree to tilt to.  
`speed`: The speed of tilting. (0-1)  

```lua
function bbmx_fx_pan(fx: string, value: integer, speed: number)
```

Pans the fixture `fx` to `value` degrees at `speed`.  
`fx`: The name of the fixture.  
`value`: The degree to pan to.  
`speed`: The speed of panning. (0-1)  

```lua
function bbmx_exit()
```

Exits the script.

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
