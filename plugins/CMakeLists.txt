set(output_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/plugins)


add_subdirectory(asciifile)
add_subdirectory(align)
add_subdirectory(divide)
add_subdirectory(gear)
add_subdirectory(importshp)
add_subdirectory(list)
add_subdirectory(picfile)
add_subdirectory(plotequation)
add_subdirectory(sameprop)
add_subdirectory(sample)

set_target_properties(asciifile align divide importshp list picfile plotequation sameprop sample PROPERTIES FOLDER plugins)

add_dependencies(asciifile librecad)
add_dependencies(divide librecad)
add_dependencies(align librecad)
add_dependencies(gear librecad)
add_dependencies(importshp librecad)
add_dependencies(list librecad)
add_dependencies(picfile librecad)
add_dependencies(plotequation librecad)
add_dependencies(sameprop librecad)
add_dependencies(sample librecad)

add_custom_target(plugins DEPENDS asciifile divide align gear importshp list picfile plotequation sameprop sample)

set(TRANSLATION_DIR ${CMAKE_CURRENT_LIST_DIR}/ts)
set(TRANSLATIONS ${TRANSLATION_DIR}/plugins_ar.ts
     ${TRANSLATION_DIR}/plugins_ca.ts
     ${TRANSLATION_DIR}/plugins_cs.ts
     ${TRANSLATION_DIR}/plugins_en.ts
     ${TRANSLATION_DIR}/plugins_en_au.ts
     ${TRANSLATION_DIR}/plugins_da.ts
     ${TRANSLATION_DIR}/plugins_de.ts
     ${TRANSLATION_DIR}/plugins_el.ts
     ${TRANSLATION_DIR}/plugins_es.ts
     ${TRANSLATION_DIR}/plugins_es_ar.ts
     ${TRANSLATION_DIR}/plugins_es_bo.ts
     ${TRANSLATION_DIR}/plugins_es_cl.ts
     ${TRANSLATION_DIR}/plugins_es_co.ts
     ${TRANSLATION_DIR}/plugins_es_cr.ts
     ${TRANSLATION_DIR}/plugins_es_do.ts
     ${TRANSLATION_DIR}/plugins_es_ec.ts
     ${TRANSLATION_DIR}/plugins_es_gt.ts
     ${TRANSLATION_DIR}/plugins_es_hn.ts
     ${TRANSLATION_DIR}/plugins_es_mx.ts
     ${TRANSLATION_DIR}/plugins_es_ni.ts
     ${TRANSLATION_DIR}/plugins_es_pa.ts
     ${TRANSLATION_DIR}/plugins_es_pe.ts
     ${TRANSLATION_DIR}/plugins_es_pr.ts
     ${TRANSLATION_DIR}/plugins_es_py.ts
     ${TRANSLATION_DIR}/plugins_es_sv.ts
     ${TRANSLATION_DIR}/plugins_es_us.ts
     ${TRANSLATION_DIR}/plugins_es_uy.ts
     ${TRANSLATION_DIR}/plugins_es_ve.ts
     ${TRANSLATION_DIR}/plugins_et.ts
     ${TRANSLATION_DIR}/plugins_eu.ts
     ${TRANSLATION_DIR}/plugins_fi.ts
     ${TRANSLATION_DIR}/plugins_fr.ts
     ${TRANSLATION_DIR}/plugins_gl.ts
     ${TRANSLATION_DIR}/plugins_hi.ts
     ${TRANSLATION_DIR}/plugins_hu.ts
     ${TRANSLATION_DIR}/plugins_id_ID.ts
     ${TRANSLATION_DIR}/plugins_it.ts
     ${TRANSLATION_DIR}/plugins_ja.ts
     ${TRANSLATION_DIR}/plugins_ko.ts
     ${TRANSLATION_DIR}/plugins_lv.ts
     ${TRANSLATION_DIR}/plugins_mk.ts
     ${TRANSLATION_DIR}/plugins_nl.ts
     ${TRANSLATION_DIR}/plugins_no.ts
     ${TRANSLATION_DIR}/plugins_pa.ts
     ${TRANSLATION_DIR}/plugins_pl.ts
     ${TRANSLATION_DIR}/plugins_pt_br.ts
     ${TRANSLATION_DIR}/plugins_pt_pt.ts
     ${TRANSLATION_DIR}/plugins_ro_ro.ts
     ${TRANSLATION_DIR}/plugins_ru.ts
     ${TRANSLATION_DIR}/plugins_sk.ts
     ${TRANSLATION_DIR}/plugins_sl.ts
     ${TRANSLATION_DIR}/plugins_sq_al.ts
     ${TRANSLATION_DIR}/plugins_sv.ts
     ${TRANSLATION_DIR}/plugins_ta.ts
     ${TRANSLATION_DIR}/plugins_tr.ts
     ${TRANSLATION_DIR}/plugins_uk.ts
     ${TRANSLATION_DIR}/plugins_zh_cn.ts
     ${TRANSLATION_DIR}/plugins_zh_tw.ts)

find_program(TRANSLATOR NAMES lrelease)

foreach(source_translations ${TRANSLATIONS})
    if(EXISTS ${source_translations})
        cmake_path(GET source_translations STEM LAST_ONLY result)
        cmake_path(GET source_translations PARENT_PATH source_path)

        set(qm_file "${source_path}/${result}.qm")
        set(qm_files ${qm_files} ${qm_file})
    endif()
endforeach()


add_custom_command(
        TARGET plugins POST_BUILD
        COMMAND ${TRANSLATOR} ${TRANSLATIONS}
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/qm
#        COMMAND ${CMAKE_COMMAND} -E copy
#            ${qm_files}
#            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/qm
#        COMMAND ${CMAKE_COMMAND} -E remove
#            ${qm_files}
)
