#!/usr/bin/env python3

import argparse;
import os;
import stat;
import zipfile;
import subprocess;

print("Welcome to evMega Firmware Update!");

def prepare_install( downloaded_pkg ):
    succ = False;
    try:
        # check exists and decompress
        zpkg = zipfile.ZipFile(downloaded_pkg);
        if zpkg.testzip() != None:
            raise Exception(downloaded_pkg + ' is not a valid zip file!');
        succ = True;
    except Exception as e:
        print('prepare_install error: ', e);
    
    return succ;

def run_installation_script( scriptPath ): 
    if os.path.exists(scriptPath):
        os.chmod(scriptPath, stat.S_IRWXU|stat.S_IRWXG|stat.S_IROTH);
        proc = subprocess.run([scriptPath], stdout=subprocess.PIPE, stderr=subprocess.PIPE);
        scriptFileName = os.path.basename(scriptPath);
        print(scriptFileName, ' output: ', proc.stdout);
        print(scriptFileName, ' error: ', proc.stderr);
        if proc.returncode != 0:
            raise Exception(scriptFileName + ' failed with code ' + str(proc.returncode) + str(proc.stdout) + str(proc.stderr));            
    else:
        raise Exception('Cannot find ' + scriptPath);

def open_package( downloaded_pkg ):
    print('Opening ', downloaded_pkg);
    # unzip package to /tmp
    zpkg = zipfile.ZipFile(downloaded_pkg);
    # run the INSTALL script from the downloaded_pkg, pass the result-file
    zpkg.extractall('/tmp');
    # the exit-code indicates pass/fail.
    # assuming the download-package has a containing folder of the same name as the package file (without the file-extension)
    # assume first entry in list of zip content is the containing folder
    contentInfoList = zpkg.infolist();
    firstItem = contentInfoList[0];
    if firstItem.filename.endswith('/'):
        zpkgName = firstItem.filename.rstrip('/');
    else: # fallback
        (zpkgName, zpkgExt) = os.path.splitext(os.path.basename(downloaded_pkg));
    pkgPath = '/tmp/' + zpkgName;
    if not os.path.exists(pkgPath):
        return None;
    return pkgPath;

    
def run_preinstall( installPkgDir ):
    installFilePath = installPkgDir + '/PREINSTALL';
    run_installation_script(installFilePath);


def run_install( installPkgDir ):
    installFilePath = installPkgDir + '/INSTALL';
    run_installation_script(installFilePath);
    # TBD: fallback if needed.


def verify_install( installPkgDir ):
    verifyInstallFilePath = installPkgDir + '/VERIFY_INSTALL';
    run_installation_script(verifyInstallFilePath);

def run_restore( installPkgDir ):
    installFilePath = installPkgDir + '/RESTORE';
    run_installation_script(installFilePath);


def displayUpdateInProgress():
    subprocess.Popen(['feh', '-F', '-Y', '/home/pi/Charger/JPG/InstallingSoftware.jpg']);

def reboot():
    os.system('sudo shutdown -r now');

"""
    2 Modes 
    1. Decompress the firmware update package, verify and mark for installation
    2. Check for installation-mark, run INSTALL, and mark success/failure
"""
"""
    program subcommand 
    subcommands (required)
    - prepare
    - install
    
    [prepare subcommand]
    arguments
    - <download-package> (required)
    - -m <marker-file> (optional, default is /home/pi/Charger/install_package_ready)
    [install subcommand]
    arguments
    - -m <marker-file> (optional, default is /home/pi/Charger/install_package_ready)
    - -r <result-file> (optional, default is /home/pi/Charger/install_result)
    - -b <backup-dir> (optional, default is /home/pi/Charger/backup_install)
    
    
"""

parser = argparse.ArgumentParser();
subparsers = parser.add_subparsers(dest='action', help='sub-command help');
parser_prep_cmd = subparsers.add_parser('prepare', help='prepare help');
parser_prep_cmd.add_argument('downloaded_pkg', help='Location of the downloaded package');
parser_prep_cmd.add_argument('-m', dest='marker_file', default='/home/pi/Charger/install_package_ready', help="marker file for installation");
parser_inst_cmd = subparsers.add_parser('install', help='install help');
parser_inst_cmd.add_argument('-m', dest='marker_file', default='/home/pi/Charger/install_package_ready', help="marker file for installation");
parser_inst_cmd.add_argument('-r', dest='result_file', default='/home/pi/Charger/install_result', help="installation result file");
args = parser.parse_args()
"""
print(args);
print(args.action);
print(args.downloaded_pkg);
print(args.marker_file);
"""


if args.action == 'prepare':
    if prepare_install(args.downloaded_pkg):
        with open(args.marker_file, 'w') as f:
            f.write(args.downloaded_pkg);
    else:
        if os.path.exists(args.marker_file):
            os.remove(args.marker_file);
elif args.action == 'install':
    shouldReboot = False;
    downloadPkgLocation = None;
    pkgPath = None;
    try:    
        if os.path.exists(args.marker_file):
            displayUpdateInProgress();
            print('Opening ', args.marker_file);
            with open(args.marker_file, 'rt') as f:
                downloadPkgLocation = f.readline();
                pkgPath = open_package(downloadPkgLocation);
                os.remove(downloadPkgLocation);
                if pkgPath:
                    run_preinstall(pkgPath);
                    run_install(pkgPath);
                    verify_install(pkgPath);
                    with open(args.result_file, 'w') as resultFile:
                        resultFile.write('OK');
                else:
                    raise Exception('Fail to open the installation package');
    except Exception as e:
        print(e);
        with open(args.result_file, 'w') as f:
            f.write('NG' + '\n');
            f.write(str(e));
        if pkgPath:
            run_restore(pkgPath);
    finally:
        if os.path.exists(args.marker_file):
            shouldReboot = True;
            print('Installation finished. Clean up ...');
            os.remove(args.marker_file);
            if downloadPkgLocation and os.path.exists(downloadPkgLocation):
                os.remove(downloadPkgLocation);
        else:
            print('No installation-package to install - Quit.');

    if shouldReboot:
        print('evMega Firmware-Update: Reboot now...');
        reboot();

