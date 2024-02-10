#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>

int
main()
{
	char *content = "content of file";
	size_t content_size = strlen(content);

	FILE *file = fopen("test_file.txt", "w+");

	if (!file) {
		printf("Fallo en la creacion del archivo\n");
		return -1;
	}

	printf("El archivo test_file.txt se crea correctamente\n");

	int bytes_written = fwrite(content, sizeof(char), content_size, file);

	if (bytes_written < 0) {
		printf("Fallo en el write\n");
		return -1;
	}

	fseek(file, 0, SEEK_SET);

	printf("Se escribieron correctamente %d bytes\n", bytes_written);
	printf("Se escribio: %s\n", content);

	char buffer[content_size];

	int bytes_read = fread(buffer, sizeof(char), content_size, file);

	if (bytes_read < 0) {
		printf("Fallo en el read\n");
		return -1;
	}

	printf("Se leyeron correctamente %d bytes\n", bytes_read);
	printf("Se leyo: %s\n", buffer);

	pclose(file);
	remove("test_file.txt");

	return 0;
}