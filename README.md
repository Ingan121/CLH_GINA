# CLH_GINA
* Fork of ConsoleLogonHook that uses resources from msgina.dll from Windows NT 4.0, 2000, or XP to create a GINA-like logon screen.

> [!WARNING]
> **THIS PROJECT IS IN EARLY DEVELOPMENT AND MIGHT BE UNSTABLE**
>
> You may encounter issues using a Microsoft account, however, this will work perfectly fine on local accounts.
>
> Disabling the lockscreen and the <kbd>CTRL</kbd> <kbd>Alt</kbd> <kbd>Del</kbd> logon keybinds is recommended as they're not fully implemented yet.
>
> **Knowing this, You might use this at your own risk.**
>

## How to contribute to the project
The following steps explain how you can contribute to the project
1. Fork this repository.
2. Pull using git commandline, or any Git UI manager (such as Github Desktop, etc.)
3. Enjoy.
 
## Installation
> [!WARNING]
> **This will require administrator privileges to be installed.**
>

* If you have installed the original ConsoleLogonHook, only replace ConsoleLogonUI.dll from this repository and proceed to step 4.

1. Copy the 2 DLL files (ConsoleLogonHook.dll and ConsoleLogonUI.dll) from [Releases](https://github.com/Ingan121/CLH_GINA/releases) into %SYSTEMROOT%\System32

2. Open a CMD window as TrustedInstaller via PsExec64 and copy and paste the following commands:

```cmd
reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Windows.Internal.UI.Logon.Controller.ConsoleBlockedShutdownResolver /v DllPath /t REG_SZ /d %systemroot%\System32\ConsoleLogonHook.dll /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Windows.Internal.UI.Logon.Controller.ConsoleLockScreen /v DllPath /t REG_SZ /d %systemroot%\System32\ConsoleLogonHook.dll /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Windows.Internal.UI.Logon.Controller.ConsoleLogonUX /v DllPath /t REG_SZ /d %systemroot%\System32\ConsoleLogonHook.dll /f

reg add HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Windows.Internal.Shell.PlatformExtensions.ConsoleCredUX /v DllPath /t REG_SZ /d %systemroot%\System32\ConsoleLogonHook.dll /f
```
or merge the regkey in the release zip as trusted installer.


3. Take ownership of the file `Windows.UI.Logon.dll` and rename it to something else. Example: `Windows.UI.Logon.dll.bak`, this is required as it will force the use of the console logon screen.

4. Get a copy of `msgina.dll` from Windows NT 4.0, 2000, or XP and place it in `%SYSTEMROOT%\System32`.

## Registry keys
### General Windows logon screen customization
* (RECOMMENDED) Disable the lockscreen
	* Create a DWORD value named `DisableLockScreen` in `HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\Personalization` and set it to `1`.
* To manually type the username, create a DWORD value named `DontDisplayLastUserName` in `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System` and set it to `1`.`
	* This is optional, as CLH_GINA handles the friendly logon as well.
* To enable verbose logon messages, create a DWORD value named `VerboseStatus` in `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System` and set it to `1`.
### Registry keys specific to CLH_GINA 
* Available at `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\LogonUI\CLH_GINA`
* 
|Name|Type|Description|Default Behavior|
|----|----|-----------|-------|
|`ShowConsole`|REG_DWORD|Set to `1` to show the console window.|Hidden|
|`ClassicTheme`|REG_DWORD|Set to `1` to make the windows use the classic theme.<br>Set to `0` to use the default theme.|Themed only when using XP msgina.dll|
|`HideStatusView`|REG_DWORD|Set to `1` to hide the status view.<br>Set to `0` to show the status view.|Hidden only when using NT4 msgina.dll|
|`CustomWallHost`|REG_SZ|Set to the path of the host process for the custom wallpaper.<br>If the value is not present, CLH_GINA's custom wallpaper implementation will be used.|CLH_GINA's custom wallpaper implementation|
|`CustomWallHostArgs`|REG_SZ|Set to the arguments for the custom wallpaper host process.|None|
|`CustomBrd`|REG_SZ|Set to the path of a BMP file to use as the small branding image.|Small branding image from msgina.dll|
|`CustomBrdLarge`|REG_SZ|Set to the path of a BMP file to use as the large branding image.|Large branding image from msgina.dll|
|`CustomBar`|REG_SZ|Set to the path of a BMP file to use as the bar image.|Bar image from msgina.dll|
### Customizing the pre-logon background and color scheme
* Color scheme: `HKEY_USERS\S-1-5-18\Control Panel\Colors`. It is recommend to run [WinClassicThemeConfig](https://gitlab.com/ftortoriello/WinClassicThemeConfig) as `NT AUTHORITY\SYSTEM` with [PsExec](https://docs.microsoft.com/en-us/sysinternals/downloads/psexec) or [gsudo](https://github.com/gerardog/gsudo) to change the color scheme of the logon screen.
* Rename `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Themes\DefaultColors\Standard` to something else to prevent reverting to the default color scheme.
* Background color: `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\Background`, default is `0 0 0` (black). Delete the value to use the background color from the color scheme.
* Background image: `HKEY_USERS\S-1-5-18\Control Panel\Desktop\Wallpaper`.
* Background image style: `HKEY_USERS\S-1-5-18\Control Panel\Desktop\WallpaperStyle`, default is `0` (centered). Set to `2` for stretched, `6` for fit, and `10` for fill.
* Set `HKEY_USERS\S-1-5-18\Control Panel\Desktop\TileWallpaper` to `1` to tile the background image.
