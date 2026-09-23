param(
    [string]$MsvcRoot = 'D:\vs2019\VC\Tools\MSVC\14.29.30133',
    [string]$QtRoot = 'D:\Qt\5.14.2\msvc2017_64',
    [string]$SdkRoot = 'C:\Program Files (x86)\Windows Kits\10',
    [string]$SdkVersion = '10.0.19041.0'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
$savedInclude = $env:INCLUDE
$savedLib = $env:LIB
$savedPath = $env:PATH
$savedPlatform = $env:QT_QPA_PLATFORM
try {
    $env:INCLUDE = "$MsvcRoot\include;$SdkRoot\Include\$SdkVersion\ucrt;$SdkRoot\Include\$SdkVersion\shared;$SdkRoot\Include\$SdkVersion\um;$QtRoot\include;$QtRoot\include\QtCore;$QtRoot\include\QtGui;$QtRoot\include\QtWidgets;$QtRoot\include\QtTest;myCode\head"
    $env:LIB = "$MsvcRoot\lib\x64;$SdkRoot\Lib\$SdkVersion\ucrt\x64;$SdkRoot\Lib\$SdkVersion\um\x64;$QtRoot\lib;lib\x64\halcon18.11"
    $env:PATH = "$QtRoot\bin;$env:PATH"
    # Native font rendering; WA_DontShowOnScreen keeps the test window invisible.
    $env:QT_QPA_PLATFORM = 'windows'
    & "$MsvcRoot\bin\Hostx64\x64\cl.exe" /nologo /utf-8 /std:c++17 /EHsc /MD /DQT_WIDGETS_LIB /DQT_GUI_LIB /DQT_CORE_LIB /DQT_TESTLIB_LIB tests\graphical_axis_panel_test.cpp /Fox64\graphical_axis_panel_test.obj /Fex64\graphical_axis_panel_test.exe /link x64\Release\graphical_program_editor.obj x64\Release\graphical_canvas.obj x64\Release\sharedFun.obj x64\Release\moc_graphical_canvas.obj x64\Release\qrc_AxisMeasurement.obj Qt5Widgets.lib Qt5Gui.lib Qt5Core.lib Qt5Test.lib halconcpp.lib delayimp.lib /DELAYLOAD:halconcpp.dll
    if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
    & .\x64\graphical_axis_panel_test.exe
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $LASTEXITCODE" }
} finally {
    $env:INCLUDE = $savedInclude
    $env:LIB = $savedLib
    $env:PATH = $savedPath
    $env:QT_QPA_PLATFORM = $savedPlatform
    Pop-Location
}
