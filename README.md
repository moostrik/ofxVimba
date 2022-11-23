# README #

OfxVimba is an Openframeworks Addon for Allied Vision GigE Cameras.
It is build and tested using Ubuntu 20.04 LTS, OpenFrameworks 0.11 and Vimba 4.2

# INSTALL LINUX #

Install Vimba for linux using the [following instructions](https://cdn.alliedvision.com/fileadmin/content/documents/products/software/software/Vimba/appnote/Vimba_installation_under_Linux.pdf "Installing Vimba under Linux")
from the [Allied Vision Site](https://www.alliedvision.com/en/products/software.html "https://www.alliedvision.com/en/products/software.html")

Copy the following files from the Vimba Folder to ofxVimba. 
* `VimbaC/Include/*` to `ofxVimba/libs/Vimba/include/VimbaC/Include/`
* `VimbaCPP/Include/*` to `ofxVimba/libs/Vimba/include/VimbaCPP/Include/`
* `VimbaCPP/DynamicLib/x86_64bit/*` to `ofxVimba/libs/Vimba/lib/linux64/`

Optional:
Optimize your ethernet adapter by setting the MTU to 8228
`sudo ifconfig eth0 mtu 8228` (where eth0 is the camera NIC)


# INSTALL WINDOWS #

Install Vimba for Windows using the installer from the [Allied Vision Site](https://www.alliedvision.com/en/products/software.html "https://www.alliedvision.com/en/products/software.html")

Copy the following files from the Vimba Folder in the Program Files to ofxVimba. 
* `VimbaC\Include\*` to `ofxVimba\libs\Vimba\include\VimbaC\Include\`
* `VimbaCPP\Include\*` to `ofxVimba\libs\Vimba\include\VimbaCPP\Include\`
* `VimbaC\Lib\Win64\*` to `ofxVimba\libs\Vimba\lib\vs\`
* `VimbaCPP\Lib\Win64\*` to `ofxVimba\libs\Vimba\lib\vs\`

After building your app with ofxVimba copy the dll files from the Vimba Folder in the Program Files `VimbaCPP\Bin\Win64\*` to the bin folder of your app 


# REFERENCES #
* [GigE Features Reference](https://www.alliedvision.com/fileadmin/content/documents/products/cameras/various/features/GigE_Features_Reference.pdf)
* [GigE Installation Manual](https://www.alliedvision.com/fileadmin/content/documents/products/cameras/various/installation-manual/GigE_Installation_Manual.pdf)

