#!/bin/bash

# Anleitung:
# Mittels dem Befehl: 'sudo chmod +x install.sh' die Installationsdatei ausfuehrbar machen
# Die Installationsdatei mittels: "./install.sh" starten

clear

echo "Herzlich wilkommen zur Installation des automatischen PGP Signierprogramm mittels GPG."
echo "Zur Installation dieses Programms benoetigen Sie:"
echo "   - eine Verbindung zum Server (konfiguration mittels ssh)"
echo "   - die Datei 'Daten.jar' (enthaelt das Signierprogramm, die index.html und die signieren.php"
echo "   - einen bereits erzeugten privaten pgp-Schluessel mit dem Namen 'mein_key.asc' (zunaechst erzeugen)"
echo "--------------------------------------------------------------------------------------"
read -p "Wollen Sie die Installation fortsetzten (y/n)? " confirm

if [ "$confirm" = "y" ]; then
	read -p "Bitte geben Sie die IP-Adresse oder den Namen des Servers an" ip
	read -p "Bitte geben Sie den vollstaendigen Pfad zur 'daten.tar' an" datenpfad
	read -p "Bitte geben Sie den vollstaendigen Pfad zur 'mein_key.asc' an" keypfad
	scp $datenpfad/Daten.tar clouduser@$ip:/home/clouduser
	scp $keypfad/0x6B7A6BBE.asc clouduser@$ip:/home/clouduser
	ssh clouduser@$ip
	
	tar xfv Daten.tar
	
	sudo bash
	apt-get update
	apt-get upgrade
	apt-get install apache2
	apt-get install php5-common libapache2-mod-php5 php5-cli
	apt-get install expect

	#http://blog.emeidi.com/2007/11/23/apache-2-2-gegen-ldap-authentifizieren/
	apt-get install slapd
	apt-get install ldap-utils
	echo "BASE    dc=hs-furtwangen,dc=de" >> /etc/ldap/ldap.conf
	echo "URI     ldaps://ldapin.informatik.hs-furtwangen.de:636" >> /etc/ldap/ldap.conf
	echo "TLS_CACERTDIR   /etc/ldap/cacerts" >> /etc/ldap/ldap.conf
	echo "TLS_REQCERT     never" >> /etc/ldap/ldap.conf

	#httpd.conf mit lokalen usern
echo "<Directory "/var/www">
        AllowOverride All
        AuthName NSG
        Allow from all
        AuthType Basic
        AuthUserFile /etc/apache2/.htpasswd
        #AuthGroupFile /etc/httpd/group
        Require valid-user
</Directory>" > /etc/apache2/httpd.conf
	htpasswd -c /etc/apache2/.htpasswd nopperbb
	#Weitere User hinzufuegen: htpasswd /etc/apache2/.htpasswd kleboths
	
	
	#index.html
	cp /home/clouduser/Daten/index.html /var/www/index.html

	#signieren.php
	cp /home/clouduser/Daten/signieren.php /var/www/signieren.php

	#impressum.html
	cp /home/clouduser/Daten/impressum.html /var/www/impressum.html

	#signierprogramm
	mkdir /var/pgp
	cp /home/clouduser/Daten/signierprogramm /var/pgp/signierprogramm
	chmod 777 /var/pgp/signierprogramm
	
	touch /var/pgp/signieren.sh
	touch /var/pgp/fingerprint
	touch /var/pgp/name
	touch /var/pgp/ldap_name
	touch /var/pgp/sign.log
	chmod 666 /var/pgp/fingerprint
	chmod 666 /var/pgp/name
	chmod 666 /var/pgp/ldap_name
	chmod 666 /var/pgp/sign.log
	chmod 777 /var/pgp/.
	chmod 777 /var/pgp/..
	chmod 777 /var/log/apache2/.
	chmod 777 /var/log/apache2/..
	chmod 777 /var/pgp/signieren.sh
	chmod 666 /var/log/apache2/access.log

	chmod 666 0x6B7A6BBE.asc
	mkdir /var/www/.gnupg
	chmod 777 /var/www/.gnupg
	chmod 777 /var/www/.gnupg/..
	chown www-data /var/www/.gnupg/.
	touch /var/www/.gnupg/gpg.conf
	chown www-data /var/www/.gnupg/gpg.conf
	echo "keyserver hkp://keys.gnupg.net" > /var/www/.gnupg/gpg.conf
	chmod 600 /var/www/.gnupg/gpg.conf

	echo "Bitte 'gpg --import mein_key.asc' eingeben und mit der Returntaste (Enter) bestaetigen"
	echo "Anschliessend bitte 'exit' eingeben und ebenfalls mit der Returntaste (Enter) bestaetigen"
	su www-data
	gpg --import mein_key.asc
	#exit
	echo "Herzlichen Glueckwunsch, ihr apache-Server und das automatische Signierprogramm wurden erfolgreich installiert"
	echo "Zum Betrieb muessen Sie den Server neustarten"
	read -p "Jetzt neustarten (y/n)? " reboot
	if [ "$reboot" = "y" ]; then
		reboot
	else
		echo "Der Serverneustart wurde nicht durchgefuehrt"
	fi
else
	echo "Die Installation wurde abgebrochen"
fi
