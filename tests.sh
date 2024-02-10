#!/bin/bash

cd prueba

touch ejemplo.txt

echo " ----------------------------"

echo " Debería estar solamente ejemplo.txt"
echo "\n"

ls

echo " ----------------------------"
echo "Mis  datos" > ejemplo.txt
echo " ----------------------------"
echo " Debería mostrar 'mis datos'"
cat ejemplo.txt
echo " ----------------------------"
echo "Deberia mostrar las estadisticas"
stat ejemplo.txt
echo " ----------------------------"

mkdir nuevo_directorio
echo " ----------------------------"
echo "Deberia aparecer ejemplo.txt y nuevo_directorio"
echo "\n"

ls
echo " ----------------------------"
cd nuevo_directorio
touch ejemplo_dentro_del_dir.txt
echo " ----------------------------"
echo "Deberia aparecer ejemplo_dentro_del_dir.txt"
echo "\n"

ls
cd ..
echo " ----------------------------"
rm ejemplo.txt
echo " ----------------------------"
echo "Deberia aparecer nuevo_directorio"
echo "\n"

ls
echo " ----------------------------"
rmdir nuevo_directorio
echo " ----------------------------"
echo "Deberia estar vacío"
echo "\n"
ls


