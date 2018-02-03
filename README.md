### Clone this project with the --recursive flag  

### Building  
It is expected to do an out of source build.
For example if the project is /project_name
Then in the parent directory have a sibling project named
/debug_project_name

Run cmake from this new directory.

To get this project up and running it has been tested in Windows 10 with Visual Studio 12 2013 with the command:  
cmake -G "Visual Studio 12 2013" ../{project_name}  

And in Ubuntu 14.10 with gcc 4.8.4 or clang 3.7.1 with the commands  
for gcc  
cmake ../{project_name}  
cmake -G "Sublime Text 2 - Unix Makefiles" ../{project_name}  
cmake -G "Sublime Text 2 - Ninja" ../{project_name}  

for clang  
CC=clang CXX=clang++ cmake ../{project_name}  
CC=clang CXX=clang++ cmake -G "Sublime Text 2 - Unix Makefiles" ../{project_name}  
CC=clang CXX=clang++ cmake -G "Sublime Text 2 - Ninja" ../{project_name}  

