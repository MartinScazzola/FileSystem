# TP4 -  FileSystem

Para la representación del sistema de archivos, se utilizó una estructura similar a la que utiliza linux:

 * Superbloque
 * Bitmap de datos
 * Bitmap de Inodos
 * Inodos
 * Bloques de datos

## <u>Superbloque</u>

```
struct superblock {
	int inodes_count;
	int data_block_count;
	int* bitmap_inodes;
    int* bitmap_data;
};
```

Se utiliza para guardar toda la información del filesystem en general.

- inodes_count: Representa la cantidad de inodos totales dentro del filesystem.
- data_block_count: Representa la cantidad de bloques de datos dentro del filesystem.
- bitmap_inodes: Representa un array que tiene una longitud igual a la cantidad de inodos, y sirve para determinar
si ese inodo está siendo utilizado o no. (inodo libre = 0, inodo ocupado = 1).
- bitmap_data: Representa un array que tiene una longitud igual a la cantidad de bloques de datos, y sirve para determinar
si ese bloque está siendo utilizado o no (bloque libre = 0, bloque ocupado = 1).


## <u>Inodo</u>

```
struct inode {
	int inode_number;
	off_t size;
	pid_t pid;
	mode_t mode;
	time_t creation_time;
	time_t modification_time;
	time_t access_time;
	char path_name[MAX_SIZE_NAME];
	uid_t user_id;
	gid_t group_id;
	int block_index;
	int type;
};
```

Se utiliza para guardar la metadata de los archivos regulares y de los directorio.

Entre estos datos se encuentran el N° de inodo, su tamaño, el pid y tiempos de creacion, modificacion, acceso.

También, para este trabajo, se utilizó la capacidad de un solo bloque de datos por inodo. Esta decisión fue arbitraria
y se realizó para facilitar el manejo del proyecto en general. Si el campo de block_index posee un -1, se concluye que
no existe información guardada en ese inodo.

Por último, también existe un campo type, que representa el tipo de archivo que se está guardando en ese inodo.
El mismo podría ser "REG_TYPE" (archivo regular = 1) o "DIR_TYPE" (directorio = 0)

## <u>Bloque de datos</u>
En nuestro proyecto, el bloque de datos es un array de 4096 caracteres, capaz de almacenar la información del input del usuario.

## <u>Pruebas</u>
Para probar el correcto funcionamiento del filesystem, se cuenta con un script llamado `tests.sh`.
Se ejecuta realizando los siguientes pasos:
1. Verificar que prueba.fisopfs NO exista en el directorio donde se corre la prueba 
2. Crear un directorio vacío llamado `prueba` donde se montará el filesystem
3. Ejecutar el comando de compilación
```bash
make
```

4. Ejecutar el comando 
```bash
./fisopfs -f ./prueba
```

5. En otra terminal, ejecutar el script con el comando 
```bash
sh tests.sh
```

6. Verificar resultados 

---

Otra metodo para testear el filesystem es mediante el archivo test01.c siguiendo los siguientes pasos

1. Verificar que prueba.fisopfs NO exista en el directorio donde se corre la prueba 
2. Crear un directorio vacío llamado `prueba` donde se montará el filesystem
3. Ejecutar el comando de compilación
```bash
make
```

4. Compilar la prueba
```bash
gcc test01.c -o test01
```

5. Ejecutar el comando 
```bash
./fisopfs -f ./prueba
```

6. En otra terminal, situado en el directorio `/prueba` ejecutar el comando
```bash
./../test01
```

## <u>Constantes</u>

|Constante|Valor|Descripción|
|-|-|-|
`MAX_SIZE_NAME`|100| Tamaño máximo de la ruta asociada a un inodo 
`MAX_FS_NAME_SIZE`|100| Tamaño máximo del nombre del archivo de persistencia
`MAX_DATA_COUNT`|1000| Cantidad máxima de bloques de datos
`MAX_SIZE_DATA_BLOCK`|4096| Tamaño de cada bloque
`MAX_INODE_COUNT`|1000|Cantidad maxima de inodos
`ALLOCATED`|1| Constante para bitmaps que representa un dato ocupado
`FREE`|0| Constante para bitmaps que representa un dato libre
`DIR_T`|0|Constante para tipo de archivo directorio
`REG_T`|1|Constante para tipo de archivo regular

