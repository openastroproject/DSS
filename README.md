DeepSkyStacker

This is the github repository for all the DeepSkyStacker source code.

DeepSkyStacker is a freeware created in 2006 by Luc Coiffier

It was open sourced in 2018 and is maintained by a small and dedicated team:

	David C. Partidge, Tony Cook, Mat Draper, Simon C. Smith, Vitali Pelenjow,
	Michal Schulz, Martin Toeltsch

The code is open source under the terms of the BSD 3-Clause License (see LICENSE).

Copyright (c) 2006-2019, LucCoiffier 
Copyright (c) 2018-2020, David C. Partridge, Tony Cook, Mat Draper,
					Simon C. Smith, Vitali Pelenjow, Michal Schulz,
					Martin Toeltsch
					
Building DeepSkyStacker:

You will need to install Visual Studio 2019 (16.11.18), Qt 5.15.2+ and Qt VS Tools 2.9.1 (rev 6) for
VS2019.

You will need to use QT VS Tools Options dialog to set up a named Qt Installation:

	Called 5.15.2x64 pointing to the msvc2019_64 sub-directory of your Qt install
		for example C:\Qt\5.15.2\msvc2019_64
		
In addition you will need to set an environment variable (right-click on the "DeepSkyStacker" project
in the VS solution explorer and go to Properties > Configuration Properties > Debugging > Environment)
called QtInstDir set to the value C:\Qt\5.15.2\msvc2019_64 (assuming you are running on 64-bit Windows).

You will also need to install [Visual Leak Detector](https://github.com/Azure/vld/releases), which
the project will expect to find at C:\Program Files (x86)\Visual Leak Detector.
In the Visual Leak Detector installation wizard, be sure to check "Add VLD directory to your environmental path".

