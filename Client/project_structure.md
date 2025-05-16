# Client Project Structure for Multi-Executable System

```
Client/
│
├── mother_bot/
│   ├── src/
│   │   └── main.cpp
│   ├── include/
│   ├── build.bat
│   └── README.md
│
├── remote_control/
│   ├── src/
│   │   └── main.cpp
│   ├── include/
│   ├── build.bat
│   └── README.md
│
├── status_retriever/
│   ├── src/
│   │   └── main.cpp
│   ├── include/
│   ├── build.bat
│   └── README.md
│
├── process_recovery/
│   ├── src/
│   │   └── main.cpp
│   ├── include/
│   ├── build.bat
│   └── README.md
│
├── common/
│   ├── src/
│   │   └── utilities.cpp
│   ├── include/
│   │   └── utilities.h
│   └── README.md
│
├── ipc/
│   ├── src/
│   │   └── ipc_manager.cpp
│   ├── include/
│   │   └── ipc_manager.h
│   └── README.md
│
└── build_all.bat
```

## Description

- Each functional module (command_executor, remote_control, status_retriever, process_recovery) is a separate executable project with its own source and include directories.
- The `common` directory contains shared utilities and helper functions used across executables.
- The `ipc` directory contains code related to inter-process communication mechanisms.
- Each module has its own build script (`build.bat`) for independent compilation.
- `build_all.bat` at the root level can be used to build all modules together.
- This structure promotes modularity, ease of development, and maintainability.
