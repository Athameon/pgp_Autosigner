<?php
error_reporting(E_ALL);
echo '<pre>';
//Der Benutzername wird aus den Logindaten ausgelesen
echo "Hallo ".$_SERVER['REMOTE_USER']."\n\n";          
if($_GET['checkbox']!="1"||$_GET['id']==""){             
  echo "Du musst alle Felder ausfuellen!";
  echo "Gehe zurueck und fuelle alle Felder aus";
}
else{
  $uid=$_SERVER['REMOTE_USER'];                       

    system("cat /var/log/apache2/access.log | grep signieren.php | tail -n2 >> /var/pgp/sign.log"); //Logging

  if(system("cat /var/pgp/sign.log | grep $uid")==""){

    $finger = $_GET['fingerprint'];
    $finger = str_replace(' ','',$finger);
    $argv = "/var/pgp/signierprogramm ".$_GET['id']." ".$finger." ".$uid;//Start von C-Programm
    $last_line = system($argv, $retval);
    if($retval==0){     //Wenn die Ueberpruefung des C-Programms ohne Fehler beendet wird, dann wir fortgesetzt.
      echo system("/var/pgp/signieren.sh", $test);
        echo $test;
      $upload = "gpg --send-key ".$_GET['id'];
      $l_line = system($upload, $retval2);
        echo $retval2;
      $antwort = "Herzlichen Glueckwunsch, dein pgp-Key wurde soeben signiert und hochgeladen";
    }
    else{
      echo "\nDie &Uumlberpruefung deiner Daten sind fehlgeschlagen\n";
    }
    echo '
    </pre>
    <hr /> ' . $antwort . '
    <hr />R&uumlckgabewert: ' . $retval;
    echo " BEACHTE: Der Upload des Schl&uumlssels kann evt. einige Stunden dauern ...";
  }
  else{
    $antwort = "Fehler: Mit deinem Logginnamen wurde bereits ein Key signiert.";
    echo '
    </pre>
    <hr /> ' . $antwort . '
    <hr />Das Signieren von mehreren Keys ist nicht m&oumlglich ' . $retval;
  }
}
?>
<a></a>
<a href="impressum.html">Impressum</a>
<input type="button" value="Startseite" onclick="window.location.href='index.html'" />

