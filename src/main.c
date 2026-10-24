#include <stdint.h>
#include <string.h>

// Definiciones de memoria para la PS4
#define PROT_READ  1
#define PROT_WRITE 2
#define PROT_EXEC  4

// Declaraciones de funciones del sistema (SDK)
extern int sceKernelUsleep(unsigned int microseconds);
extern int sceKernelMprotect(const void *addr, size_t len, int prot);

// Variables de estado del mando (simuladas de tu código original)
int xbox_device_connected = 0;
int xbox_data_ready = 0;

// Puntero a la función original
int (*original_scePadRead)(int handle, void *pData, int num) = NULL;

// Función donde procesas tus datos del USB
void apply_xbox_input_to_ps4_buffer(void* pData) {
    // Aquí va tu lógica original para mezclar los botones (LX, LY, RX, RY, etc.)
}

// 1. Solución al Error 0x80240007
int my_scePadRead(int handle, void *pData, int num) {
    // Evitar crasheos si la función original no está resuelta
    if (!original_scePadRead) return -1;

    // Llamar a la función original de la PS4
    int ret = original_scePadRead(handle, pData, num);

    // Si la lectura original devuelve error (ej. 0x80240007), pausar y no mezclar
    if (ret < 0) {
        sceKernelUsleep(1000); 
        return ret;
    }

    // Si todo está bien y el mando está listo, inyectar los datos
    if (xbox_device_connected && xbox_data_ready) {
        apply_xbox_input_to_ps4_buffer(pData);
    }

    return ret;
}

// 2. Solución a la discrepancia de memoria (Hooking directo)
void patch_function_jmp(uintptr_t target_addr, void* hook_func) {
    if (!target_addr) return;

    // Alinear a la página de memoria (4KB) y desproteger
    uintptr_t page_start = target_addr & ~0xFFF;
    sceKernelMprotect((void*)page_start, 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC);

    // Instrucción de salto absoluto para x86_64 (JMP [RIP+0] + Dirección 64-bit)
    uint8_t jmp_code[] = {
        0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, // jmp QWORD PTR [rip+0]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Espacio para la dirección
    };

    // Insertar la dirección de nuestra función hook en el espacio vacío
    *(uintptr_t*)(&jmp_code[6]) = (uintptr_t)hook_func;

    // Escribir los bytes en la memoria del juego
    memcpy((void*)target_addr, jmp_code, sizeof(jmp_code));

    // Restaurar los permisos de la memoria por seguridad
    sceKernelMprotect((void*)page_start, 0x1000, PROT_READ | PROT_EXEC);
}

// Punto de entrada del plugin
int module_start(size_t args, const void *argp) {
    // Direcciones internas obtenidas de tu log
    uintptr_t scePadRead_internal = 0x8004c4ea0;
    
    // (Opcional) Aquí deberías inicializar original_scePadRead a un trampolín 
    // que ejecute los bytes originales sobreescritos.
    
    // Aplicar el parche
    patch_function_jmp(scePadRead_internal, my_scePadRead);
    
    return 0;
}

// Punto de salida del plugin
int module_stop(size_t args, const void *argp) {
    return 0;
}
