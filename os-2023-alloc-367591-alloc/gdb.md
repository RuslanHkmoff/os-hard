# Настройка GDB

Мы ожидаем, что вы выполнили все инструкции из первого задания — если нет, начните с них.

Вам понадобится версия `gdb` с таргет-платформой `riscv64`. Она, скорее всего, называется либо `gdb-multiarch`, либо `riscv64-вашаархитектура-gdb`. Поищите её там, где установлен ваш тулчейн или в `$PATH`.

Запустите в одном окне терминала `make qemu-gdb`, а в другом — из корневой директории проекта `ваш-gdb -ix .gdbinit kernel/kernel`.

У вас, вероятно, появятся предупреждения о невозможности автозагрузки файла — их можно игнорировать. Нажмите Enter, а затем введите `frame`. Вы увидите текущую исполняемую инструкцию — по умолчанию в режиме отладки в xv6 устанавливается брейкпоинт на первой инструкции:

```
(gdb) frame
#0  0x0000000000001000 in ?? ()
=> 0x0000000000001000:  97 02 00 00     auipc   t0,0x0
```

Введите команду `c` (contniue — продолжить). Вернитесь в терминал с xv6 и убедитесь, что операционная система запустилась и вы попали в терминал. Чтобы выйти из GDB, введите команду `q`.

Однако, мы понимаем, что отлаживать в GDB не очень удобно. Ниже мы рассмотрим способы включить отладчик в популярных IDE и редакторах.

## JetBrains CLion

Добавьте в «Run / Debug Configurations» конфигурацию «GDB Remote Debug». Установите следующие опции:

* GDB — путь к вашему GDB для RISC-V
* 'target remote' args — возьмите в файле `.gdbinit`, имеет вид `127.0.0.1:1234`
* Symbol file — путь к файлу `kernel/kernel` в вашем проекте
* Sysroot — путь к вашему проекту

Для отладки нужно сначала запустить `make qemu-gdb` в терминале, а потом начать отладку в CLion.

## Visual Studio Code

Установите расширение [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) от Microsoft. Это можно сделать из маркетплейса или командой `ext install ms-vscode.cpptools` (нажмите Ctrl+P, чтобы ввести эту команду).

Создайте в корне вашего проекта директорию `.vscode`, а внутри — файл `launch.json`. Укажите в нём следующую конфигурацию:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug xv6 Kernel",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/kernel/kernel",
            "cwd": "${workspaceFolder}",
            "stopAtEntry": true,
            "MIMode": "gdb",
            "miDebuggerServerAddress": "127.0.0.1:26000", // Укажите хост и порт из .gdbinit
            "miDebuggerPath": "/usr/bin/gdb-multiarch", // Укажите путь к вашему GDB для RISC-V
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```

Для отладки нужно сначала запустить `make qemu-gdb` в терминале, а потом нажать F5 (Run → Start Debugging) в VS Code.
Мы не рекомендуем запускать саму xv6 во встроенном терминале, поскольку терминал некорректно обрабатывает некоторые нажатия клавиш (например, Ctrl + P).

## Другие редакторы

Попробуйте найти плагин для вашего редактора, который позволяет работать с GDB. 

Если такого нет, то для многих редакторов (например, ViM) существуют плагины, которые поддерживают DAP — Debug Adapter Protocol. Расширение для VS Code как раз реализует DAP-адаптер для GDB, поэтому некоторые плагины используют его. Например, [vimspector](https://github.com/puremourning/vimspector#c-c-rust-etc).

Соответственно, конфиг можно взять тот же.

Если вы нашли хорошее решение — поделитесь им с однокурсниками в чате.