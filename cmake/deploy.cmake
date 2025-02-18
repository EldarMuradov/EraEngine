function(era_add_deploy_target app)
    set(deploy_target_name deploy_modules_to_${app})
    add_custom_target(${deploy_target_name} ALL)

    get_property(target_dependencies TARGET ${app} PROPERTY DEPENDENCIES)
    list(REMOVE_DUPLICATES target_dependencies)

    foreach(dependency ${target_dependencies})
        get_property(dependency_type TARGET ${dependency} PROPERTY TARGET_TYPE)
        if(dependency_type STREQUAL "MODULE")
            add_custom_command(TARGET ${deploy_target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                $<TARGET_FILE_DIR:${dependency}>
                $<TARGET_FILE_DIR:${app}>
            )
        endif()
    endforeach()
    
    set_target_properties(${deploy_target_name} PROPERTIES FOLDER deployment)
endfunction()