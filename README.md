# README #

OfxVimba is an Openframeworks Addon for Allied Vision GigE Cameras.

It is build and tested for Ubuntu 20.04 LTS using Qt Creator 8, OpenFrameworks 0.11.2 and Vimba 6.0

It is build and tested for Windows 10 using Visual Studio 2022, OpenFrameworks 0.11.2 and Vimba 6.0

Vimba does not support MacOS


In earlier versions of this addon it was possible to set all the features for the cameras from the addon.
This proved quite complicated, as a lot of features are interdepended and many features require the device not to be streaming.
It proved easier to set the features using the SDK's Vimba Viewer and save these settings in the availabe UserSets.
The addon can load these usersets with the loadUserSet() command.
It is also possible to open the camera in ReadOnly mode to immediately see these changes within your app. (Make sure multicast is enabled though) 
Although it is still possible to set features from the addon, such as framerate and exposure, succes will depend on other features.
For example, setting the framerate will only have an effect when 'AcquisitionMode' is set to 'Continuous' and 'TriggerSource' to 'FixedRate'

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

