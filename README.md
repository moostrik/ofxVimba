# README #

OfxVimba is an Openframeworks Addon for Allied Vision GigE Cameras.
It is build and tested using Ubuntu 20.04 LTS, OpenFrameworks 0.11 and Vimba 4.2

# INSTALL #

Install Vimba for linux using the [following instructions](https://cdn.alliedvision.com/fileadmin/content/documents/products/software/software/Vimba/appnote/Vimba_installation_under_Linux.pdf "Installing Vimba under Linux")
from the [Allied Vision Site](https://www.alliedvision.com/en/products/software.html "https://www.alliedvision.com/en/products/software.html")

Copy the following files from Vimba to ofxVimba. 
* `/Vimba_4_2/VimbaC/Include/*` to `/ofxVimba/libs/Vimba/include/VimbaC/Include/`
* `/Vimba_4_2/VimbaCPP/Include/*` to `/ofxVimba/libs/Vimba/include/VimbaCPP/Include/`
* `/Vimba_4_2/VimbaCPP/DynamicLib/x86_64bit/*` to `/ofxVimba/libs/Vimba/lib/linux64/`

Optional:
Optimize your ethernet adapter by setting the MTU to 8228
`sudo ifconfig eth0 mtu 8228` (where eth0 is the camera NIC)

# REFERENCES #
* [GigE Features Reference](https://www.alliedvision.com/fileadmin/content/documents/products/cameras/various/features/GigE_Features_Reference.pdf)
* [GigE Installation Manual](https://www.alliedvision.com/fileadmin/content/documents/products/cameras/various/installation-manual/GigE_Installation_Manual.pdf)

