// Imports :-)
var fs = new ActiveXObject("Scripting.FileSystemObject");
var shell = new ActiveXObject("WScript.Shell");
var shellApp = new ActiveXObject("Shell.Application");

var debug = false;

function debugPrint(message) {
    if (debug) {
        WScript.Echo(message);
    }
}

function createProxyInstallerScript(proxyScriptName, realScriptName) {
    var proxyTextStream = fs.CreateTextFile(proxyScriptName);
    proxyTextStream.WriteLine("@" + realScriptName);
    proxyTextStream.Close();
}

function findProjectTopDir(scriptName) {
    var script = fs.GetFile(fs.GetAbsolutePathName(scriptName));
    var binDir = script.ParentFolder;
    var topDir = binDir.ParentFolder;
    return topDir.Path;
}

function getVersion(exe) {
    var process = shell.Exec(exe);
    var firstLine = process.StdOut.ReadLine();
    /(v\d+\.\d+\.\d+),/.exec(firstLine);
    return RegExp.$1;
}

function makeTempFolder() {
    var TEMP_DIR = 2;
    var temp = fs.BuildPath(fs.GetSpecialFolder(TEMP_DIR), fs.GetTempName());
    return fs.CreateFolder(temp); // Returns a Folder object
}

function createParentFolders(path) {
    var parts = path.split('\\');
    var dir = parts[0] + "\\"; // BuidPath won't work with the drive letter.

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

// Create temporary hierarchy of only what is to be zipped so it can be
// zipped all in one operation. That's the only way to get subfolders 
// into the zip file.
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
        var zipFile = fs.GetFile(zipFileName);            // zipFile is type File
        var zipFolder = shellApp.NameSpace(zipFile.Path); // zipFolder is type Folder
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

