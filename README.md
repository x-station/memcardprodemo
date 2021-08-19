# memcardprodemo

This minimal setup demonstrates how to configure the PSX pad SPI registers to send arbitrary commands.
It then sends some MemcardPro commands that change the active memory card on it.

# Installation

	Brief:

		Install docker
		Whitelist this folder *
		Run the .bat

		If you don't have this option, you're using an up to date version of Win10 and can skip step 2!
		* Settings (icon) -> Resources -> File Sharing -> Little blue (+) icon -> add the folder

	Instructions:

		Install this: https://www.docker.com/

		Goto Settings | Resources | File Sharing, and add this folder

		Run buildme.bat to build.

		Docker will do a one-time download of the build image (see intro) and then do the thing.
		
