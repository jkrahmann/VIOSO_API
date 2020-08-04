# VIOSO WarpBlend API

This library is meant to use in image generators, to do warping and blending. It takes a .vwf export and a texture buffer 
to sample from. If no texture buffer is given, it uses a copy of the current back buffer. It will render to the currently set back buffer.
It provides image based warping, suitable for most cases and, if a 3D map is provided, dynamc eye warping.

## Usage: 

### 1. Static binding
having VIOSOWarpBlend next to your executable or in added to %path%:
link against VIOSOWarpBlend.lib and, in your [precompiled] header.
#include "VIOSOWarpBlend.h"

### 2. Dynamic binding
giving a path to load dynamic library from:
#define VIOSOWARPBLEND_FILE with a path (relative to main executable), this defaults to VIOSOWarpBlend / VIOSOWarpBlend64

### 2a) via [precompiled] header:
in header declare functions and types:
#define VIOSOWARPBLEND_DYNAMIC_DEFINE
#include "VIOSOWarpBlend.h"

in one file on top, to implement the actual functions/objects
#define VIOSOWARPBLEND_DYNAMIC_IMPLEMENT
#include "VIOSOWarpBlend.h"

in module initialization, this loads function pointers from library
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "VIOSOWarpBlend.h"

---
### 2b) Single file
in file on top, to declare and implement functions/objects,
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "VIOSOWarpBlend.h"

in module initialization, this loads function pointers from library
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "VIOSOWarpBlend.h"

Always make sure to have your platform headers loaded before!
To access deprecated functions #define VWB_USE_DEPRECATED_INIT

### @Remarks
To allocate a warper, call VWB_Create. A specified config file is read.
You might edit all settings in warper struct. Then call VWB_Init.
There is no logical maximum on the number of allocated warpers.
Call VWB_getViewProj to obtain view and projection matrices relative to some eye point.
Call VWB_render to warp scene to screen. There is no multi-threading inside VIOSOWarpBlend module, make sure to use same thread or make your GL-context current before calling VWB_render.

You need VIOSO Calibrator to export a warp map as Vioso Warp File.
OR you can download mapings here:
https://vioso.sharepoint.com/:f:/s/intern/EnmNHs_Y7r1Lno7zCl2y1CsB8vIpWHsQdV9ZMK2OQNjXOQ?e=jRzHTk

This API can use all mappings. In case you export 3D, you have to specify view parameter.
The eye point correction algorithm works implicitly. Imagine a rectangular "window" placed virtually near next to the screen.
It must be situated the way that from every possible (dynamic) eye point the whole projection area of the regarded projector
is seen through tat window.
This leads to an asymetrical frustum for a dynamic eye-point, which leads us to a projection-view-matrix mPV.
If you use that matrix in conjunction with the world matrix, one can render this "window" to provide all information needed
on the screen for that projector.
The map provided for the projector has 3D-coordinates of the actual screen for every pixel. If you multiply such a 
3D coordinate with mPV it leads to the u-v-coordinate in the "window". So if we sample in the provided backbuffer,
we can fill every pixel with content.
Here we define the rectangle. We do that in the familiar way of defining a standard frustum. The only additional information s the 
distance, where the screen will be (screen). These values come from a configuraton file or can be entered in settings.
Example .ini file (comments are actually not allowed there, remove them before use!!):
; this is where values for every channel go...
[default]
logLevel=1							;log level, 0 only fatal errors, 1 only errors and important info, 2 normal log, but guaranteed no info logging in render and frustum methods, 3 verbose debug log, defaults to 0
logFile=VIOSOWarpBlend.log			;some other log file; it defaults to VIOSOWarpBlend.log, relative paths are relative to VIOSOWarpBlend.dll, not the main module!
bLogClear=0							;if 1, clear the logfile on start, defaults to 0
near=1.0							;the near plane 
far=20000.0							;the far plane
trans=[1,0,0,0;0,1,0,0;0,0,1,0;0,0,0,1] ;this is the transformation matrix transforming a vioso coordinate to an IG coordinate this defines the pivot of a moving platform, column major format
base=[1,0,0,0;0,1,0,0;0,0,-1,0;0,0,0,1] ;same as above, row major format DirectX conform, as DX uses left-handed coordinate system, z has to be inverted! 
bTurnWithView=1						;set to 1 if view turnes and moves with eye, i.e view is obtained from a vehicle position on a moving platform, defaults to 0
bBicubic=0							;set to 1 to enable bicubic content texture filter, defaults to 0
bDoNotBlend=0						;set to 1 to disable blending, defaults to 0
splice=0							;bitfield:	0x00000001 change sign of pitch, 0x00000002 use input yaw as pitch, 0x00000004 use input roll as pitch
									;0x00000010 change sign of yaw, 0x00000020 use input pitch as yaw, 0x00000040 use input roll as yaw
									;0x00000100 change sign of roll, 0x00000200 use input pitch as roll, 0x00000400 use input yaw as roll
									;0x00010000 change sign of x movement, 0x00020000 use input y as x, 0x00040000 use input z as x
									;0x00100000 change sign of y movement, 0x00200000 use input x as y, 0x00400000 use input z as y
									;0x01000000 change sign of z movement, 0x02000000 use input x as z, 0x04000000 use input y as z
bAutoView=0							;set to 1 to enable automatic view calculation, defaults to 0
									;it will override dir=[], fov=[] and screen to use calculated values
autoViewC=1							;moving range coefficient. defaults to 1, set a range in x,y,z from platform origin to be mapped; x+- autoViewC * screen / 2
gamma=1.0							;set a gamma value. This is multiplied by the gamma already set while calibrating!, defaults to 1, if calibrators blend looks different than in your application, adjust this value!
port=942							;set to a TCP IPv4 port, use 0 to disable network. defaults to 0, standard port is 942
addr=0.0.0.0						;set to IPv4 address, to listen to specific adapter, set to 0.0.0.0 to listen on every adapter, defaults to 0.0.0.0
bUseGL110=0							;set to 1, to use shader version 1.1 with fixed pipeline, defaults to 0
bPartialInput						;set to 1, to input texture act as optimal rect part, defaults to 0
eye=[0,0,0]							; this is the eye position relative to pivot, defaults to [0,0,0]
dir=[0, 0, 0]						; this is the view direction, rotation angles around axis' [x,y,z]. The rotation order is yaw (y), pitch (x), roll (z); positive yaw turns right, positive pitch turns up and positive roll turns clockwise, defaults to [0,0,0]
fov=[45,45,45,45]					; this is the fields of view, x - left, y - top, z - right, w - bottom, defaults to [30,20,30,20]
screen=3.650000000					; the distance of the render plane from eye point.defaults to 1, watch the used units
calibFile=..\Res\Calib_150930.vwf ; path to warp blend file(s), if relative, it is relative to the VIOSOWarpBlend.dll, not the main module! To load more than .vwf/.bmp, separate with comma.
calibIndex=0						; index, in case there are more than one display calibrated; if index is omitted you can specify the display ordinal
calibAdapterOrdinal=1				; display ordinal the number in i.e. "D4 UHDPROJ (VVM 398)" matches 4. In case no index is found index is set to 0
eyePointProvider=EyePointProvider	; a eye point provider dll name; the name is passed to LoadLibrary, so a full qualified path is possible, use quotes if white spaces
eyePointProviderParam=				; string used to initialize provider, this depends on the implmementation
mouseMode=0							; bitfield, set 1 to render current mouse cursor on top of warped buffer

[channel 1]
eye=[0,0,0]
dir=[0, -20, 0]
fov=[45,25,45,25]
screen=3.000000
calibFile=..\Res\Calib_150930_1.vwf
calibIndex=0
;[channel 2] next channel, if you are using this .ini for more than one channel
