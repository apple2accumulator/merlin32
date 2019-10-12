//////////////////////////////////////////////////////////////////////////
//
// Builds a zip file that can be used to install merlin32 on Windows.
//
// Author: Bill Chatfield
//
// Designed to be run from the Makefile in Source directory as follows:
//     (Start a Visual C++ Command Prompt)
//     cd merlin32/Source
//     nmake installer
//
// But can be run via:
//     cscript make-installer.js
//
// Of course it won't work if you run it before its dependencies in
// the Makefile.
//
// This is written in Microsoft's JScript, which is part of Windows
// Script Host that is included in every copy of Windows. The advantange
// is that the JavaScript language is much
// cleaner and easier to read than either PowerShell or Batch files,
// which are the only other options for built-in scripting in Windows.
// Actually, VBScript is another option but it is significantly slower
// than JScript. Maximum usability means zero dependencies.
//
//////////////////////////////////////////////////////////////////////////

// System COM objects:
// These are being treated like imports, meaning that they are global
// variables. This approach has these advantages:
// 1. Creating the objects only once saves CPU time compared to creating
//    separate instances in each function that requires them.
// 2. They do not clutter up function argument lists.
//    Don't over-react and claim that this implies that all variables
//    should be global. Only variables that represent global data
//    should be global.
// 3. As global variables they better model the global nature of the
//    functionality they provide.
var fs = new ActiveXObject("Scripting.FileSystemObject");
var shell = new ActiveXObject("WScript.Shell");
var shellApp = new ActiveXObject("Shell.Application");

/**
 * Determines whether to output debug messages or not.
 */
var debug = false;

/**
 * Outputs a message only if debug is turned on.
 *
 * @param {String} message - What it is
 */
function debugPrint(message) {
    if (debug)
        WScript.Echo(message);
}

/**
 * Creates a batch file in the top directory that can call the
 * real install script.
 *
 * @param {String} proxyScriptName - Name of script to create
 * @param {String} realScriptName - Name of script it should call
 */
function createProxyInstallerScript(proxyScriptName, realScriptName) {
    var writer = fs.CreateTextFile(proxyScriptName);
    writer.WriteLine("@" + realScriptName);
    writer.Close();
}

/**
 * Finds the absolute path name of this script's parent's parent
 * directory.
 *
 * @param {String} scriptName - Just that
 * @return {String} Absolute path of top directory
 */
function findProjectTopDir(scriptName) {
    var script = fs.GetFile(fs.GetAbsolutePathName(scriptName));
    var binDir = script.ParentFolder;
    var topDir = binDir.ParentFolder;
    return topDir.Path;
}

/**
 * Runs the main executable and collects the version number from its
 * output.
 *
 * @param {String} exe - The path to the executable to be run
 * @return {String} The version number
 */
function getVersion(exe) {
    var process = shell.Exec(exe); // Type is WshScriptExec
    var firstLine = process.StdOut.ReadLine();
    var matches = /(v\d+\.\d+\.\d+),/.exec(firstLine); // Type is Array
    if (matches == null)
        throw new Error("Failed to get version number from " + exe);
    return matches[1]; // First parenthesized substring match
}

/**
 * Creates a temporary folder according to the OS standard conventions.
 *
 * @return {Folder} The newly created folder
 */
function makeTempFolder() {
    var TEMP_DIR = 2;
    var tmp = fs.BuildPath(fs.GetSpecialFolder(TEMP_DIR), fs.GetTempName());
    return fs.CreateFolder(tmp); // Returns a Folder object
}

/**
 * Creates all directories in a path that do not already exist. If
 * the last component of path is a folder, it is not created. Only
 * the parents are created. This is necessary.
 *
 * @param {String} path - The path containing folders to create
 */
function createParentFolders(path) {
    var parts = path.split('\\');
    var dir = parts[0] + "\\"; // BuildPath won't work with drive letter.

    debugPrint(parts);

    for (var i = 1; i < parts.length - 1; i++) {

        debugPrint("part=" + parts[i]);
        if (dir.length == 0)
            dir = parts[i];
        else
            dir = fs.BuildPath(dir, parts[i]);

        if (fs.FolderExists(dir)) {
            debugPrint("Folder exists: " + dir);
            continue;
        }
        else {
            debugPrint("Creating folder: " + dir);
            fs.CreateFolder(dir);
        }
    }
}

/**
 * Copies a file to a destination creating any folders necessary to build
 * the destination location.
 *
 * @param {String} src - Name of source file
 * @param {String} dest - Name of destination file
 */
function copyFile(src, dest) {
    debugPrint("cwd=" + shell.CurrentDirectory);
    debugPrint("Copying " + src + " to " + dest);
    createParentFolders(dest);
    if (fs.FolderExists(src))
        fs.CopyFolder(src, dest, true);
    else
        fs.CopyFile(src, dest, true);
}

/**
 * Creates a temporary hierarchy of only what is to be zipped so it can be
 * zipped all in one operation. That's the only way to get subfolders 
 * into the zip file.
 *
 * @param {String} topDirName - Name to be given to the directory in
 *                              the zip file which will contain all the
 *                              provided files
 * @param {Array of String} filesToZip - Relative paths to all files
 *                                       to be zipped
 * @return {Folder} The temporary directory in which the hierarchy was
 *                  created
 */
function createHierarchyForZip(topDirName, filesToZip) {
    // tempDir will receive a Folder object
    var tempDir = makeTempFolder();
    // targetTopDir will receive a Folder object
    var targetTopDir = tempDir.SubFolders.Add(topDirName);
    for (var i = 0; i < filesToZip.length; i++) {
        var target = fs.BuildPath(targetTopDir, filesToZip[i]);
        copyFile(filesToZip[i], target);
    }
    return tempDir;
}

/**
 * Zips a group of files.
 *
 * @param {String} zipFileName - Name to be given to the zip file
 * @param {String} topDirName - Name to be given to the directory in
 *                              the zip file which will contain all the
 *                              provided files
 * @param {Array of String} filesToZip - Relative paths to all files
 *                                       to be zipped
 */
function zip(zipFileName, topDirName, filesToZip) {
    // Create empty ZIP file and open for adding
    var zipTextStream = fs.CreateTextFile(zipFileName, true);
    zipTextStream.Write("PK\05\06\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0");
    zipTextStream.Close();

    // tempDir will receive a Folder object
    var tempDir = createHierarchyForZip(topDirName, filesToZip);

    // Use a try/finally to make sure the tempDir gets deleted in case of
    // any errors.
    try {
        var zipFile = fs.GetFile(zipFileName); // zipFile is type File
        var zipFolder = shellApp.NameSpace(zipFile.Path); // type is Folder
        if (zipFolder == null) {
            throw new Error(0, "Failed to create Zip file: " + zipFileName);
        }

        WScript.Echo("Zipping " + tempDir.Path);
        zipFolder.CopyHere(fs.BuildPath(tempDir.Path, topDirName));
        WScript.Sleep(5000); // REQUIRED!! (Depending on file/dir size)
    }
    finally {
        debugPrint("Deleting temp directory: " + tempDir);
        tempDir.Delete();
    }
}

//////////////////////////////////////////////////////////////////////////
//
// Restarts this script with cscript.exe if we're running wscript.exe so
// that we can write to stdout rather than displaying a separate popup
// for each WScript.Echo.
//
// WScript is always started in a new window, so there is no way to get
// back to the original Command Prompt if this was started from within
// a Command Prompt. All we can do is make sure it is run in a new
// Command Prompt if it was double-clicked from the desktop.
//
// TODO: Testing!
//
//////////////////////////////////////////////////////////////////////////
if (WScript.FullName.search(/WScript/i) >= 0) {
    // Get a "WScript.Shell" object if we don't already have one.
    var mineShell = (typeof shell === 'object') && ('Environment' in shell)
        ? shell : new ActiveXObject("WScript.Shell");
    // The ComSpec environment variable stores the system command line 
    // shell.
    var restartSelfCmd = 'cscript //nologo "' + WScript.ScriptFullName+'"';
    // Run the command and collect the exit code.
    var exitCode = mineShell.Run(restartSelfCmd, 8, true);
    WScript.Quit(exitCode);
}

//////////////////////////////////
//
// Main Program
//
//////////////////////////////////

// Change the directory to this project's top directory.
shell.CurrentDirectory = findProjectTopDir(WScript.ScriptFullName);
// Must be some way to convert from absolute to relative here but not sure
// yet how. We need the relative paths for "zip" to work the way it is
// expected.
var sourceDir = "Source";
var scriptsDir = "Scripts";
var binaryExecutable = fs.BuildPath(sourceDir, "merlin32.exe");
var version = getVersion(binaryExecutable);
var installerDir = "Installer";
if (! fs.FolderExists(installerDir)) {
    fs.CreateFolder(installerDir);
}

var distZip = fs.BuildPath(installerDir, "merlin32-" + version + "-windows-installer.zip");
var proxyInstallScript = "run-this-to-install.bat";
var realInstallScript = fs.BuildPath(scriptsDir, "install.bat");
createProxyInstallerScript(proxyInstallScript, realInstallScript);

zip(distZip, "merlin32", [
    proxyInstallScript,
    binaryExecutable,
    "README.md",
    fs.BuildPath(scriptsDir, "addtopath.bat"),
    fs.BuildPath(scriptsDir, "uninstall.bat"),
    realInstallScript,
    "Library"
]);

WScript.Echo("Created " + distZip);
fs.DeleteFile(proxyInstallScript);

