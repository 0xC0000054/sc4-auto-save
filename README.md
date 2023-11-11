# sc4-auto-save

A DLL Plugin for SimCity 4 that automatically saves a city at user-specified intervals.    
The plugin can be downloaded from the Releases tab: https://github.com/0xC0000054/sc4-auto-save/releases

## System Requirements

* Windows 10 or later

## Installation

1. Close SimCity 4.
2. Copy `SC4AutoSave.dll` and `SC4AutoSave.ini` into the Plugins folder in the SimCity 4 installation directory.
3. Configure the auto-save settings, see the `Configuring the plugin` section.

## Configuring the plugin

1. Open `SC4AutoSave.ini` in a text editor (e.g. Notepad).    
Note that depending on the permissions of your SimCity 4 installation directory you may need to start the text editor 
with administrator permissions to be able to save the file.

2. Adjust the settings in the `[AutoSave]` section to your preferences.
3. Save the file and start the game.

## Troubleshooting

The plugin should write a `SC4AutoSave.log` file in the same folder as the plugin.    
The log contains status information for the most recent run of the plugin.

# License

This project is licensed under the terms of the MIT License.    
See [LICENSE.txt](LICENSE.txt) for more information.

## 3rd party code

[gzcom-dll](https://github.com/nsgomez/gzcom-dll/tree/master) Located in the vendor folder, MIT License.    
[Windows Implementation Library](https://github.com/microsoft/wil) MIT License
[.NET runtime](https://github.com/dotnet/runtime) The `Stopwatch` class is based on `System.Diagnostics.Stopwatch`, MIT License.
[Boost.PropertyTree](https://www.boost.org/doc/libs/1_83_0/doc/html/property_tree.html) Boost Software License, Version 1.0.

# Source Code

## Prerequisites

* Visual Studio 2022

## Building the plugin

* Open the solution in the `src` folder
* Update the post build events to copy the build output to you SimCity 4 application plugins folder.
* Build the solution

## Debugging the plugin

Visual Studio can be configured to launch SimCity 4 on the Debugging page of the project properties.
I configured the debugger to launch the game in a window with the following command line:    
`-intro:off -CPUcount:1 -w -CustomResolution:enabled -r1920x1080x32`

You may need to adjust the resolution for your screen.
