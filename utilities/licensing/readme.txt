
$Id: readme.txt 2487 2012-05-03 21:14:57Z chenli $

// CHEN LI

This package includes a tool kit to generate a license key for the Bimaple engine.

To compile it:

 shell> make build
 shell> cd build
 shell> cmake ..
 shell> make

To run the binary package to generate a new license key, do the following command
in the "build" folder:

 shell> ./licensecreator ../key/rsakey.pem name=bimaple_license,Expiry-Date=2012-08-01

The string after "Expiry-Date" specifies the expiration date.

Copy and paste the output text

Signature=i/t1+I08bu/rOU3P/DueG6cl1EwtUXmmvzny/b4RUoQepfPQTOUfB5gcpjRSh6x6UXeZYk0cKkT0maMH3Favvwx1mJHn45pGW4GBP0LVpBfTpzf8DH1At+bAP2lf/lsqsVfvdzTdaQfhmXvjDLtkTaM0bgyQXu0/nRG3h6RBH4c=,name=bimaple_license,Expiry-Date=2012-08-01

to a file as the license key.


