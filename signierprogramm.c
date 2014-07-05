/*
 ===================================================================================================================
 Name        : signierprogramm.c
 Author      : Benjamin Nopper
 Version     : 0.9
 Copyright   : LGPL TODO: Welches Copyright ist das Richtige?
 Description : Dieses Programm signiert pgp-Schluessel automatisch ueber einen apache2-Server.
 			   Die Authentifizierung und Authorisierung erfolgt ueber einen ldap-Server.
 			   
 			   Dem Programm muss per ARGV folgende werte mitgegeben werden:
 			   1. ID des Schluessels in der Form: 0x6B7A6BBE *
 			   2. Fingerprint ohne trennende Leerzeichen: 715A07FA1DF1DC11E1F230A31F3FBCD06B7A6BBE *
 			   3. UID des ldap		*das sind die Daten meines PGP-Schluessels, welcher gerne unterschieben werden darf
 ===================================================================================================================
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void loesche_eindimensionale_Arrays(char* name_unmodifiziert, char* fingerprint, char* eigener_fingerprint, char* gesamtname, char* ldap_name);
int testn_der_Dateien();
void pgp_Befehle_aufrufen(char* id);
void speichert_ldap_name_in_Datei(char* uid);
int fingerprint_aus_Datei_auslesen(char* fingerprint);
void name_aus_Datei_auslesen(char* name_unmodifiziert);
int vergleiche_fingerprint(char* fingerprint, char* eigener_fingerprint, int laenge);
void ldap_name_aus_Datei_auslesen(char* ldap_name);
void entferne_Zwischenzeichen_aus_Fingerprint(char* argv_fingerprint, char* eigener_fingerprint, int laenge);
int modifiziere_und_vergleiche_Namen(char* ldap_name, char* gesamtname, char* name_unmodifiziert);
void signing_sh_anlegen(char* id);
void name_loggen(char* gesamtname);

int main(int argc, char*argv[]) {
	char fingerprint[100], eigener_fingerprint[100];
	char name_unmodifiziert[100], gesamtname[100], ldap_name[100];
	int laenge=0;
	char* id = argv[1];
	char* uid = argv[3];
	char* argv_fingerprint = argv[2];
	
	loesche_eindimensionale_Arrays(name_unmodifiziert, fingerprint, eigener_fingerprint, gesamtname, ldap_name);
	
	//Wenn die Dateien zum Zwischenspeichern der ausgelesenen datein schreibgeschützt sind, dann wird das Programm mit dem Fehler '1' beendet
	if(testn_der_Dateien() == 1)
		return 1;
	
	pgp_Befehle_aufrufen(id);
	speichert_ldap_name_in_Datei(uid);
	laenge = fingerprint_aus_Datei_auslesen(fingerprint);
	name_aus_Datei_auslesen(name_unmodifiziert);
	ldap_name_aus_Datei_auslesen(ldap_name);
	entferne_Zwischenzeichen_aus_Fingerprint(argv_fingerprint, eigener_fingerprint, laenge);

	//Wenn der eingegebene Fingerprint nicht mit dem aus dem public-Key ausgelesenen Fingerprint uebereinstimmt, dann wird das Programm mit dem Fehler '3' beendet
	if(vergleiche_fingerprint(fingerprint, eigener_fingerprint, laenge)==3)
		return 3;
	
	//Wenn nicht mindestens 2 Teilnamen uebereinstimmen, dann wird das Programm mit dem Fehler '2' beendet
	if(modifiziere_und_vergleiche_Namen(ldap_name, gesamtname, name_unmodifiziert)<2)
		return 2;
	
	signing_sh_anlegen(id);
	name_loggen(gesamtname);
	
	//Wenn die Textdateien lesbar, der Fingerprint richtig und mindestens 2 Namen richtig sind, dann wird das Programm normal mit '0' beendet
	return 0;
}

void loesche_eindimensionale_Arrays(char* name_unmodifiziert, char* fingerprint, char* eigener_fingerprint, char* gesamtname, char* ldap_name){
	int i;
	for(i=0; i<100; i++){	//Die Inhalte der Char-Arrays werden geloescht.
		name_unmodifiziert[i] = ' ';
		fingerprint[i] = ' ';
		eigener_fingerprint[i] = ' ';
		gesamtname[i] = ' ';
		ldap_name[i] = ' ';
	}
}

int testn_der_Dateien(){ //Testet ob die Dateien fingerprint und name und ldap_name nicht schreibgeschuetzt sind (Manipulationsschutz).
	FILE *lese1_test = fopen("fingerprint", "w");
	FILE *lese2_test = fopen("name", "w");
	FILE *lese3_test = fopen("ldap_name", "w");
	FILE *lese4_test = fopen("signieren.sh", "w");
	if(!(lese1_test && lese2_test && lese3_test && lese4_test)){
		printf("Fehler beim Zugriff auf die Dateien 'fingerprint', 'name' oder 'ldap_name' - Der Vorgang wurde abgebrochen");
		return 1;
	}
	fclose(lese1_test);
	fclose(lese2_test);
	fclose(lese3_test);
	fclose(lese4_test);
	
	return 0;
}

void pgp_Befehle_aufrufen(char* id){
	//Laedt den oeffentlichen Schluessel vom Keyserver und importiert diesen.
	char syscall_null[30] = "gpg --recv-keys ";
	strcat(syscall_null, id);
	system(syscall_null);

	//Speichert unmodifizierter Fingerprint in die Datei fingerprint.
	char syscall_eins[55] = "gpg --fingerprint "; 
	strcat(syscall_eins, id); strcat(syscall_eins, " | grep = > fingerprint");
	system(syscall_eins);

	//Speichert unmodifizierter Name aus dem Schluessel in die Datei name 
	char syscall_name[60] = "gpg --fingerprint ";	
	strcat(syscall_name, id); strcat(syscall_name, " | grep uid | head -1 > name");
	system(syscall_name);
}

void speichert_ldap_name_in_Datei(char* uid){
	//Speichert unmodifizierter Name aus einem ldap aufruf in die Datei ldap-name
	char syscall_ldap_name[60] = "ldapsearch -x uid=";	
	strcat(syscall_ldap_name, uid); strcat(syscall_ldap_name, " | grep displayName: > ldap_name");
	system(syscall_ldap_name);
}

int fingerprint_aus_Datei_auslesen(char* fingerprint){
	int i, j, start=0;
	char c, fingerprint_unmodifiziert[100];
	for(i=0; i<100; i++)	//Die Inhalte des Arrays werden geloescht.
		fingerprint_unmodifiziert[i] = ' ';
	
	//Liest den Fingerprint wieder aus der Datei "fingerprint" aus und in das Programm ein.
	FILE *fingerprint_datei = fopen("fingerprint", "r");
	if (fingerprint_datei) {
		for(i=0; (c = getc(fingerprint_datei)) != EOF; i++){
			fingerprint_unmodifiziert[i] = c;
		}
	} else {
		printf("Error opening file!\n");
		exit(1);
	}
	fclose(fingerprint_datei);
	
	//Entfernt alle zeichen vor dem gespiecherten (unmodifizierten) Fingerprint und speichert den reinen Fingerprint in einem neuen Array.
	for(i=0, j=0; i<100; i++){
		if(start == 1 && fingerprint_unmodifiziert[i] != ' '){
			fingerprint[j] = fingerprint_unmodifiziert[i];
			j++;
		}
		if(fingerprint_unmodifiziert[i] == '=')
			start = 1;
	}
	
	return --j;
}

void name_aus_Datei_auslesen(char* name_unmodifiziert){
	//Liest den aus dem Schluessel ausgelesenen Namen wieder aus der Datei "name" aus und in das Programm ein.
	int i; char c;
	FILE *name_datei = fopen("name", "r");
	if (name_datei) {
		for(i=0; (c = getc(name_datei)) != EOF; i++){
			name_unmodifiziert[i] = c;
		}
	} else {
		printf("Error opening file!\n");
		exit(1);
	}
}

void ldap_name_aus_Datei_auslesen(char* ldap_name){
	//Liest den aus dem ldap ausgelesenen Namen wieder aus der Datei "ldap_name" aus und in das Programm ein.
	int i; char c;
	FILE *ldap_name_datei = fopen("ldap_name", "r");
	if (ldap_name_datei) {
		for(i=0; (c = getc(ldap_name_datei)) != EOF; i++){
			ldap_name[i] = c;
		}
	} else {
		printf("Error opening file!\n");
		exit(1);
	}
	fclose(ldap_name_datei);
}

void entferne_Zwischenzeichen_aus_Fingerprint(char* argv_fingerprint, char* eigener_fingerprint, int laenge){
	//Entfernt die Zwischenzeichen ("-", "_") des eigegebenen Fingerprints fuer den reinen Fingerprint.
	int i, j;
	for(i=0, j=0; i<100; i++){
		if(!(argv_fingerprint[i] == '-' || argv_fingerprint[i] == '_')&& j<laenge){	
			eigener_fingerprint[j] = toupper(argv_fingerprint[i]);
			j++;
		}
	}
}

int vergleiche_fingerprint(char* fingerprint, char* eigener_fingerprint, int laenge){
	//Ueberprueft den eingegebenen Fingerprint mit dem aus dem Schluessel ausgelesenen Fingerprint.
	int i;
	for(i=0; i<laenge-1; i++){
		
		if(fingerprint[i] != eigener_fingerprint[i]){
			printf("FEHLER: Der aus dem Schlüssel errechnete Fingerprint stimmt nicht mit dem Eingegebenen ueberein\n");
			return 3;
		}
	}
	
	return 0;
}

int modifiziere_und_vergleiche_Namen(char* ldap_name, char* gesamtname, char* name_unmodifiziert){
	char name[5][20], ldap_namen[5][20];
	int z=0,i,k,m,j,l, anz_r_namen;
	for(i=0; i<5; i++){
		for(j=0; j<20; j++){
			name[i][j] = ' ';
			ldap_namen[i][j] = ' ';
		}
	}
	//Kopiert den Gesamtnamen vom ldap_name-Array in jeweils einzelne ldap_namen_array.
	for(i=0, j=-1; i<100; i++){
		for(k=0;k<20; k++, i++){
			if(ldap_name[i] == ' '){
				j++;
				break;
			}
			else if(j>=0){
				ldap_namen[j][k] = ldap_name[i];
			}
		}
	}

	//Kopiert und modifiziert den den unmodifizierten Namen in das gesamtname-Array und das zweidimensionale namen-Array.
	for(i=10, k=0, l=0; i<100; i++){
		if(name_unmodifiziert[i] != ' '){
			for(j=0; j<100; j++, i++, l++){
				gesamtname[j] = toupper(name_unmodifiziert[i]);
				if(gesamtname[j] == ' '){
					k++;
					l=-1;
				}
				else{
					name[k][l] = gesamtname[j];
					
				}
				if(name_unmodifiziert[i+2] == '<'){
					j=100;
					i=100;
				}
			}
		}
	}
	
	/*
	* Definition: Teilname = Vorname, zweiter Vorname, Nachname, ...
	* Ueberprueft, ob der mindestnes zwei Namen aus aus dem Schluessel und dem eingegebenen/aus dem ldap ausgelesenen Namen uebereinstimmt.
	* Dabei wird immer jeder ldap-Teilname mit genau einem Teilnamen des public-Keys ueberprueft.
	* Das Ergebnis ist positiv, wenn mindestens zwei Teilnamen uebereinstimmen. Z.B.: 1. Vorname + Nachname, 1. Vorname + Nachname, ...
	* Gefahr: im ldap sind zwei Vornamen vorhanden und der public Key hat nur die zwei Vornamen (kein Nachname). TODO: Gibt es bei ldap zwei Vornmen????
	* GRUND: Die ueberpuefung ist aufgrund von eventuell auftretenden 2. Vornamen notwendig
	* k= ldap-Namen; m= Schluesselnamen
	*/
	
	for(k=0; k<=5; k++){	//Namesnwörter von ldap
		for(m=0; m<=5; m++){	//Vergleich verschiedener Wörter von Name des Schluessels
			for(i=0, z=0 ;z==0;i++){	//Buchstabendurchlauf
				if(name[m][i]==' '){
					z=1;
					m=10;	//Sobald ein (Teil-)Name erfolgreich ueberpruft wurde wird der naechste Teilname vom ldap ueberprueft.
					break;
				}
				if(toupper(ldap_namen[k][i]) != name[m][i] && z==0){
					z=1;
				}
				else{
					if(name[m][i+1] == ' '){
						anz_r_namen++;
					}
				}
			}	
		}
	}
	if(anz_r_namen<2) printf("FEHLER: Der in Felix hinterlegte Name stimmt nicht mit dem Namen des public-Keys ueberein");
	
	return anz_r_namen;
}

void signing_sh_anlegen(char* id){
	FILE *signier_und_save_aufruf = fopen("signieren.sh", "w");
		fprintf(signier_und_save_aufruf, "#!/usr/bin/expect\nspawn gpg --edit-key ");
		fprintf(signier_und_save_aufruf, id);
		fprintf(signier_und_save_aufruf, " sign save\n");
		fprintf(signier_und_save_aufruf, "expect \"Really sign all user IDs? (y/N)\"\nsend -- \"y\\n\"\n");
		fprintf(signier_und_save_aufruf, "expect \"Really sign? (y/N)\"\nsend -- \"y\\n\"\n");
		fprintf(signier_und_save_aufruf, "expect \"Really sign? (y/N)\"\nsend -- \"\\n\"\n\ninteract");
	fclose(signier_und_save_aufruf);
	
	printf("Herzlichen Glueckwunsch. Deine Logindaten, dein Name und dein Fingerprint sind richtig.\n");
}

void name_loggen(char* gesamtname){
	//Loggt den aus dem public Key ausgelesenen Namen.
	FILE *log_datei = fopen("sign.log", "a+");
	if (log_datei) {
		fprintf(log_datei,"%s\n",gesamtname);
	} else {
		printf("Error opening Logfile!\n");
		exit(1);
	}
	fclose(log_datei);
}
