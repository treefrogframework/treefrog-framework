---
title: Deployment
page_id: "130.0"
---

## Deployment

The developed application is deployed in the production environment (or test environment) and is then run.

Although it’s easier if the source code is built in the production environment, in general the production environment and the build machine are separate. For building, the computer needs to have the same OS/Library installed as in the production environment. The binary for release can then be built. The binary which is produced, and the related files are then transferred from the archive to the production environment.
 
### Release Mode Building

In order to build the source code in release mode, the following command should be run in the application root. A binary that is optimized to the environment is then generated.

```
 $ qmake -r "CONFIG+=release" 
 $ make clean
 $ make
```

### Deploying to the production environment

First, check the settings for the production environment. You should check the user name/password in the [*product*] section in the database.ini settings file, and the number of the listening port in the *application.ini* file. Please ensure these are all correct for your environment.

The following is a summary of the files needed for the application to work correctly. All the files and subdirectories will be archived in the following directory:

* config
* db      ← not necessary if sqlite is not used
* lib
* plugin
* public
* sql

Examples of tar command:

```
 $ tar cvfz app.tar.gz  config/  db/  lib/  plugin/  public/  sql/
```

- Please change the tar file name as appropriate.

This then is the construction in a production environment. Building and configuration of the database systems, and installation of TreeFrog Framework/Qt having been completed in advance.

The tar file is copied to the production environment. Once copied, it can then be expanded by creating a directory.

```
 $ mkdir app_name
 $ cd app_name
 $ tar xvfz (ディレクトリ名)/app.tar.gz
```

To start, use the following command by specifying the application root directory (must be the absolute path) as a command option.

```
 $ sudo treefrog -d  [application_root_path]
```

Some distributions may require you to have root privileges if you want to open port 80. In this example, I started with the sudo command.

In addition, in Linux, you will be able to activate the app automatically by making the init.d script. In Windows, this is possible by registering the startup. Because there are many articles on the net about how to start the service automatically at OS boot time, I don’t need to go into detail here.

The next statement shows the Stop Command.

```
 $ sudo treefrog -k stop [application_root_path]
```