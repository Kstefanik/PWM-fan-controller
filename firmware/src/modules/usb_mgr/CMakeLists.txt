target_sources(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/usb_mgr.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usbd_init.c
        ${CMAKE_CURRENT_SOURCE_DIR}/usb_handlers.c
        
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/usb_msg_decode.c
        ${CMAKE_CURRENT_SOURCE_DIR}/generated/usb_msg_encode.c
)

target_include_directories(app PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
)