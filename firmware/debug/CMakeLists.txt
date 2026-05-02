function(generate_ozone_project board device)
    set(OZONE_BOARD ${board})
    set(OZONE_DEVICE ${device})
    set(OZONE_ELF_RELATIVE_PATH "$(ProjectDir)/../build/${board}/zephyr/zephyr.elf")

    configure_file(
            ${CMAKE_CURRENT_LIST_DIR}/ozone-project.jdebug.in
            ${CMAKE_CURRENT_LIST_DIR}/${board}.jdebug
            @ONLY
    )
endfunction()

generate_ozone_project(nucleo_l476rg STM32L476RG)
generate_ozone_project(nucleo_f446ze STM32F446ZE)
generate_ozone_project(stm32f4_disco STM32F407VG)
generate_ozone_project(nucleo_h533re STM32H533RE)
