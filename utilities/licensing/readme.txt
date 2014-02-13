
$Id: readme.txt 2487 2012-05-03 21:14:57Z chenli $

// CHEN LI

This package includes a tool kit to generate a license key for the Bimaple engine.

To compile it:

 shell> mkdir build
 shell> cd build
 shell> cmake ..
 shell> make

To run the binary package to generate a new license key, do the following command
in the "build" folder:

 shell> ./licensecreator ../key/rsakey.pem name=srch2_license_key.txt,Expiry-Date=2013-12-01

The string after "Expiry-Date" specifies the expiration date.

Copy and paste the output text to a file as the license key.


