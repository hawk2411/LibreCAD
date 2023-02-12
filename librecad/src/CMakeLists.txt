# LibreCAD project file
# (c) Ries van Twisk (librecad@rvt.dds.nl)
include(boost.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
option(DISABLE_POSTSCRIPT "" false)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fext-numeric-literals")

#uncomment to enable a Debugging menu entry for basic unit testing
add_compile_definitions(LC_DEBUGGING)
add_compile_definitions(DWGSUPPORT)
add_compile_definitions(JWW_WRITE_SUPPORT)
add_compile_definitions(QC_APPDIR="\"librecad\"")

set(LC_VERSION "2.2.0-alpha")
set(VERSION ${LC_VERSION})

#find_package(GTest REQUIRED)
find_package(Qt5 REQUIRED PrintSupport Widgets Svg )
#find_package(Qt5Core REQUIRED)
#find_package(Qt5Gui REQUIRED)
#find_package(Qt5Widgets REQUIRED)
##find_package(Qt5PrintSupport REQUIRED)
#find_package(Qt5Svg REQUIRED)

if(NOT TRANSLATOR)
    message(FATAL_ERROR "Qt translator 'lrelease' not found")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


#if( ${build_muparser} )
#    message("build_muparser is on")
#    include_directories()
#endif()

set(CMAKE_AUTOUIC_SEARCH_PATHS ui ui/forms)

set(SOURCES
        ui/lc_actionfactory.cpp
        ui/qg_actionhandler.cpp
        ui/qg_blockwidget.cpp
        ui/qg_colorbox.cpp
        ui/qg_commandedit.cpp
        ui/qg_dialogfactory.cpp
        ui/qg_filedialog.cpp
        ui/qg_fontbox.cpp
        ui/qg_graphicview.cpp
        ui/qg_layerbox.cpp
        ui/qg_layerwidget.cpp
        ui/qg_librarywidget.cpp
        ui/qg_linetypebox.cpp
        ui/qg_patternbox.cpp
        ui/qg_pentoolbar.cpp
        ui/qg_recentfiles.cpp
        ui/qg_widthbox.cpp
        ui/lg_dimzerosbox.cpp
        ui/qg_scrollbar.cpp
        ui/forms/qg_arcoptions.cpp
        ui/forms/qg_arctangentialoptions.cpp
        ui/forms/qg_beveloptions.cpp
        ui/forms/qg_blockdialog.cpp
        ui/forms/qg_circleoptions.cpp
        ui/forms/qg_circletan2options.cpp
        ui/forms/qg_commandwidget.cpp
        ui/forms/qg_coordinatewidget.cpp
        ui/forms/qg_dimensionlabeleditor.cpp
        ui/forms/qg_dimlinearoptions.cpp
        ui/forms/qg_dimoptions.cpp
        ui/forms/qg_dlgarc.cpp
        ui/forms/qg_dlgattributes.cpp
        ui/forms/qg_dlgcircle.cpp
        ui/forms/qg_dlgdimension.cpp
        ui/forms/qg_dlgdimlinear.cpp
        ui/forms/qg_dlgellipse.cpp
        ui/forms/qg_dlghatch.cpp
        ui/forms/qg_dlgimage.cpp
        ui/forms/qg_dlgimageoptions.cpp
        ui/forms/qg_dlginitial.cpp
        ui/forms/qg_dlginsert.cpp
        ui/forms/qg_dlgline.cpp
        ui/forms/qg_dlgmirror.cpp
        ui/forms/qg_dlgmove.cpp
        ui/forms/qg_dlgmoverotate.cpp
        ui/forms/qg_dlgmtext.cpp
        ui/forms/qg_dlgoptionsdrawing.cpp
        ui/forms/qg_dlgoptionsgeneral.cpp
        ui/forms/qg_dlgoptionsmakercam.cpp
        ui/forms/qg_dlgpoint.cpp
        ui/forms/qg_dlgpolyline.cpp
        ui/forms/qg_dlgrotate.cpp
        ui/forms/qg_dlgrotate2.cpp
        ui/forms/qg_dlgscale.cpp
        ui/forms/qg_dlgspline.cpp
        ui/forms/qg_dlgtext.cpp
        ui/forms/qg_exitdialog.cpp
        ui/forms/qg_imageoptions.cpp
        ui/forms/qg_insertoptions.cpp
        ui/forms/qg_layerdialog.cpp
        ui/forms/qg_libraryinsertoptions.cpp
        ui/forms/qg_lineangleoptions.cpp
        ui/forms/qg_linebisectoroptions.cpp
        ui/forms/qg_lineoptions.cpp
        ui/forms/qg_lineparalleloptions.cpp
        ui/forms/qg_lineparallelthroughoptions.cpp
        ui/forms/qg_linepolygon2options.cpp
        ui/forms/qg_linepolygonoptions.cpp
        ui/forms/qg_linerelangleoptions.cpp
        ui/forms/qg_modifyoffsetoptions.cpp
        ui/forms/qg_mousewidget.cpp
        ui/forms/qg_moverotateoptions.cpp
        ui/forms/qg_mtextoptions.cpp
        ui/forms/qg_polylineoptions.cpp
        ui/forms/qg_polylineequidistantoptions.cpp
        ui/forms/qg_printpreviewoptions.cpp
        ui/forms/qg_roundoptions.cpp
        ui/forms/qg_selectionwidget.cpp
        ui/forms/qg_snapdistoptions.cpp
        ui/forms/qg_snapmiddleoptions.cpp
        ui/forms/qg_splineoptions.cpp
        ui/forms/qg_textoptions.cpp
        ui/forms/qg_trimamountoptions.cpp
        ui/forms/qg_widgetpen.cpp
        ui/lc_centralwidget.cpp
        ui/lc_widgetfactory.cpp
        ui/twostackedlabels.cpp
        ui/qg_commandhistory.cpp
        ui/lc_dockwidget.cpp
        ui/forms/lc_dlgsplinepoints.cpp
        ui/forms/lc_widgetoptionsdialog.cpp
        ui/forms/qg_snaptoolbar.cpp
        ui/forms/qg_activelayername.cpp
        ui/lc_deviceoptions.cpp
        ui/generic/comboboxoption.cpp
        ui/generic/actionlist.cpp
        ui/generic/widgetcreator.cpp
        ui/lc_actiongroupmanager.cpp
        ui/generic/linklist.cpp
        ui/generic/colorcombobox.cpp
        ui/generic/colorwizard.cpp
        ui/lc_penwizard.cpp
        ui/generic/textfileviewer.cpp
        ui/forms/qg_commandwidget.ui
        ui/forms/qg_arcoptions.ui
        ui/forms/qg_arctangentialoptions.ui
        ui/forms/qg_beveloptions.ui
        ui/forms/qg_blockdialog.ui
        ui/forms/qg_circleoptions.ui
        ui/forms/qg_circletan2options.ui
        ui/forms/qg_coordinatewidget.ui
        ui/forms/qg_dimensionlabeleditor.ui
        ui/forms/qg_dimlinearoptions.ui
        ui/forms/qg_dimoptions.ui
        ui/forms/qg_dlgattributes.ui
        ui/forms/qg_dlghatch.ui
        ui/forms/qg_dlginitial.ui
        ui/forms/qg_dlginsert.ui
        ui/forms/qg_dlgimage.ui
        ui/forms/qg_dlgimageoptions.ui
        ui/forms/qg_dlgarc.ui
        ui/forms/qg_dlgcircle.ui
        ui/forms/qg_dlgdimension.ui
        ui/forms/qg_dlgdimlinear.ui
        ui/forms/qg_dlgline.ui
        ui/forms/qg_dlgellipse.ui
        ui/forms/qg_dlgmirror.ui
        ui/forms/qg_dlgmove.ui
        ui/forms/qg_dlgmoverotate.ui
        ui/forms/qg_dlgmtext.ui
        ui/forms/qg_dlgoptionsdrawing.ui
        ui/forms/qg_dlgoptionsgeneral.ui
        ui/forms/qg_dlgoptionsmakercam.ui
        ui/forms/qg_dlgpoint.ui
        ui/forms/qg_dlgpolyline.ui
        ui/forms/qg_dlgrotate.ui
        ui/forms/qg_dlgrotate2.ui
        ui/forms/qg_dlgscale.ui
        ui/forms/qg_dlgspline.ui
        ui/forms/qg_dlgtext.ui
        ui/forms/qg_exitdialog.ui
        ui/forms/qg_imageoptions.ui
        ui/forms/qg_insertoptions.ui
        ui/forms/qg_layerdialog.ui
        ui/forms/qg_libraryinsertoptions.ui
        ui/forms/qg_lineangleoptions.ui
        ui/forms/qg_linebisectoroptions.ui
        ui/forms/qg_lineoptions.ui
        ui/forms/qg_lineparalleloptions.ui
        ui/forms/qg_lineparallelthroughoptions.ui
        ui/forms/qg_linepolygon2options.ui
        ui/forms/qg_linepolygonoptions.ui
        ui/forms/qg_linerelangleoptions.ui
        ui/forms/qg_modifyoffsetoptions.ui
        ui/forms/qg_mousewidget.ui
        ui/forms/qg_moverotateoptions.ui
        ui/forms/qg_mtextoptions.ui
        ui/forms/qg_polylineoptions.ui
        ui/forms/qg_polylineequidistantoptions.ui
        ui/forms/qg_printpreviewoptions.ui
        ui/forms/qg_roundoptions.ui
        ui/forms/qg_selectionwidget.ui
        ui/forms/qg_snapdistoptions.ui
        ui/forms/qg_snapmiddleoptions.ui
        ui/forms/qg_splineoptions.ui
        ui/forms/qg_textoptions.ui
        ui/forms/qg_trimamountoptions.ui
        ui/forms/qg_widgetpen.ui
        ui/forms/qg_snaptoolbar.ui
        ui/forms/qg_activelayername.ui
        ui/forms/lc_dlgsplinepoints.ui
        ui/forms/lc_widgetoptionsdialog.ui
        ui/lc_deviceoptions.ui
        ui/generic/comboboxoption.ui
        ui/generic/widgetcreator.ui
        ui/generic/colorwizard.ui
        ui/generic/textfileviewer.ui
        lib/actions/rs_actioninterface.cpp
        lib/actions/rs_preview.cpp
        lib/actions/rs_previewactioninterface.cpp
        lib/actions/rs_snapper.cpp
        lib/actions/RS_SnapMode.cpp
        lib/actions/RS_SnapMode.h
        lib/creation/rs_creation.cpp
        lib/debug/rs_debug.cpp
        lib/engine/rs_arc.cpp
        lib/engine/rs_block.cpp
        lib/engine/rs_blocklist.cpp
        lib/engine/rs_clipboard.cpp
        lib/engine/rs_circle.cpp
        lib/engine/rs_constructionline.cpp
        lib/engine/rs_dimaligned.cpp
        lib/engine/rs_dimangular.cpp
        lib/engine/rs_dimdiametric.cpp
        lib/engine/rs_dimension.cpp
        lib/engine/rs_dimlinear.cpp
        lib/engine/rs_dimradial.cpp
        lib/engine/rs_document.cpp
        lib/engine/rs_ellipse.cpp
        lib/engine/rs_entity.cpp
        lib/engine/rs_entitycontainer.cpp
        lib/engine/rs_font.cpp
        lib/engine/rs_fontlist.cpp
        lib/engine/rs_graphic.cpp
        lib/engine/rs_hatch.cpp
        lib/engine/lc_hyperbola.cpp
        lib/engine/rs_insert.cpp
        lib/engine/rs_image.cpp
        lib/engine/rs_layer.cpp
        lib/engine/rs_layerlist.cpp
        lib/engine/rs_leader.cpp
        lib/engine/rs_line.cpp
        lib/engine/rs_mtext.cpp
        lib/engine/rs_overlayline.cpp
        lib/engine/rs_overlaybox.cpp
        lib/engine/rs_pattern.cpp
        lib/engine/rs_patternlist.cpp
        lib/engine/rs_point.cpp
        lib/engine/rs_polyline.cpp
        lib/engine/rs_settings.cpp
        lib/engine/rs_solid.cpp
        lib/engine/rs_spline.cpp
        lib/engine/lc_splinepoints.cpp
        lib/engine/rs_system.cpp
        lib/engine/rs_text.cpp
        lib/engine/rs_undo.cpp
        lib/engine/rs_undoable.cpp
        lib/engine/rs_units.cpp
        lib/engine/rs_utility.cpp
        lib/engine/rs_variabledict.cpp
        lib/engine/rs_vector.cpp
        lib/fileio/rs_fileio.cpp
        lib/filters/rs_filtercxf.cpp
        lib/filters/rs_filterdxfrw.cpp
        lib/filters/rs_filterdxf1.cpp
        lib/filters/rs_filterjww.cpp
        lib/filters/rs_filterlff.cpp
        lib/gui/rs_dialogfactory.cpp
        lib/gui/rs_eventhandler.cpp
        lib/gui/rs_graphicview.cpp
        lib/gui/rs_grid.cpp
        lib/gui/rs_linetypepattern.cpp
        lib/gui/rs_painter.cpp
        lib/gui/rs_painterqt.cpp
        lib/gui/rs_staticgraphicview.cpp
        lib/information/rs_locale.cpp
        lib/information/rs_information.cpp
        lib/information/rs_infoarea.cpp
        lib/math/rs_math.cpp
        lib/math/lc_quadratic.cpp
        lib/modification/rs_modification.cpp
        lib/modification/rs_selection.cpp
        lib/engine/rs_color.cpp
        lib/engine/rs_pen.cpp
        actions/lc_actiondrawcircle2pr.cpp
        test/lc_simpletests.cpp
        lib/generators/lc_xmlwriterqxmlstreamwriter.cpp
        lib/generators/lc_makercamsvg.cpp
        actions/lc_actionfileexportmakercam.cpp
        lib/engine/rs_atomicentity.cpp
        lib/engine/rs_undocycle.cpp
        lib/engine/rs_flags.cpp
        lib/engine/lc_rect.cpp
        lib/engine/lc_undosection.cpp
        lib/engine/rs.cpp
        lib/printing/lc_printing.cpp
        actions/lc_actiondrawlinepolygon3.cpp

        actions/rs_actionblocksadd.cpp
        actions/rs_actionblocksattributes.cpp
        actions/rs_actionblockscreate.cpp
        actions/rs_actionblocksedit.cpp
        actions/rs_actionblockssave.cpp
        actions/rs_actionblocksexplode.cpp
        actions/rs_actionblocksinsert.cpp
        actions/rs_actionblocksfreezeall.cpp
        actions/rs_actionblocksremove.cpp
        actions/rs_actionblockstoggleview.cpp
        actions/rs_actiondefault.cpp
        actions/rs_actiondimaligned.cpp
        actions/rs_actiondimangular.cpp
        actions/rs_actiondimdiametric.cpp
        actions/rs_actiondimension.cpp
        actions/rs_actiondimleader.cpp
        actions/rs_actiondimlinear.cpp
        actions/rs_actiondimradial.cpp
        actions/rs_actiondrawarc.cpp
        actions/rs_actiondrawarc3p.cpp
        actions/rs_actiondrawarctangential.cpp
        actions/rs_actiondrawcircle.cpp
        actions/rs_actiondrawcircle2p.cpp
        actions/rs_actiondrawcircle3p.cpp
        actions/rs_actiondrawcirclecr.cpp
        actions/rs_actiondrawcircleinscribe.cpp
        actions/rs_actiondrawcircletan1_2p.cpp
        actions/rs_actiondrawcircletan2_1p.cpp
        actions/rs_actiondrawcircletan2.cpp
        actions/rs_actiondrawcircletan3.cpp
        actions/rs_actiondrawellipseaxis.cpp
        actions/rs_actiondrawellipsefocipoint.cpp
        actions/rs_actiondrawellipse4points.cpp
        actions/rs_actiondrawellipsecenter3points.cpp
        actions/rs_actiondrawellipseinscribe.cpp
        actions/rs_actiondrawhatch.cpp
        actions/rs_actiondrawimage.cpp
        actions/rs_actiondrawline.cpp
        actions/rs_actiondrawlineangle.cpp
        actions/rs_actiondrawlinebisector.cpp
        actions/rs_actiondrawlinefree.cpp
        actions/rs_actiondrawlinehorvert.cpp
        actions/rs_actiondrawlineparallel.cpp
        actions/rs_actiondrawlineparallelthrough.cpp
        actions/rs_actiondrawlineorthtan.cpp
        actions/rs_actiondrawlinepolygon.cpp
        actions/rs_actiondrawlinepolygon2.cpp
        actions/rs_actiondrawlinerectangle.cpp
        actions/rs_actiondrawlinerelangle.cpp
        actions/rs_actiondrawlinetangent1.cpp
        actions/rs_actiondrawlinetangent2.cpp
        actions/rs_actiondrawmtext.cpp
        actions/rs_actiondrawpoint.cpp
        actions/rs_actiondrawpolyline.cpp
        actions/rs_actiondrawspline.cpp
        actions/lc_actiondrawsplinepoints.cpp
        actions/rs_actiondrawtext.cpp
        actions/rs_actioneditcopy.cpp
        actions/rs_actioneditpaste.cpp
        actions/rs_actioneditundo.cpp
        actions/rs_actionfilenew.cpp
        actions/rs_actionfilenewtemplate.cpp
        actions/rs_actionfileopen.cpp
        actions/rs_actionfilesave.cpp
        actions/rs_actionfilesaveas.cpp
        actions/rs_actioninfoangle.cpp
        actions/rs_actioninfoarea.cpp
        actions/rs_actioninfoinside.cpp
        actions/rs_actioninfodist.cpp
        actions/rs_actioninfodist2.cpp
        actions/rs_actioninfototallength.cpp
        actions/rs_actionlayersadd.cpp
        actions/rs_actionlayersedit.cpp
        actions/rs_actionlayersfreezeall.cpp
        actions/rs_actionlayerslockall.cpp
        actions/rs_actionlayersremove.cpp
        actions/rs_actionlayerstogglelock.cpp
        actions/rs_actionlayerstoggleview.cpp
        actions/rs_actionlayerstoggleprint.cpp
        actions/lc_actionlayerstoggleconstruction.cpp
        actions/rs_actionlibraryinsert.cpp
        actions/rs_actionlockrelativezero.cpp
        actions/rs_actionmodifyattributes.cpp
        actions/rs_actionmodifybevel.cpp
        actions/rs_actionmodifycut.cpp
        actions/rs_actionmodifydelete.cpp
        actions/rs_actionmodifydeletefree.cpp
        actions/rs_actionmodifydeletequick.cpp
        actions/rs_actionmodifyentity.cpp
        actions/rs_actionmodifymirror.cpp
        actions/rs_actionmodifymove.cpp
        actions/rs_actionmodifymoverotate.cpp
        actions/rs_actionmodifyoffset.cpp
        actions/rs_actionmodifyrevertdirection.cpp
        actions/rs_actionmodifyrotate.cpp
        actions/rs_actionmodifyrotate2.cpp
        actions/rs_actionmodifyround.cpp
        actions/rs_actionmodifyscale.cpp
        actions/rs_actionmodifystretch.cpp
        actions/rs_actionmodifytrim.cpp
        actions/rs_actionmodifytrimamount.cpp
        actions/rs_actionmodifyexplodetext.cpp
        actions/rs_actionoptionsdrawing.cpp
        actions/rs_actionorder.cpp
        actions/rs_actionpolylineadd.cpp
        actions/rs_actionpolylineappend.cpp
        actions/rs_actionpolylinedel.cpp
        actions/rs_actionpolylinedelbetween.cpp
        actions/rs_actionpolylinetrim.cpp
        actions/rs_actionpolylineequidistant.cpp
        actions/rs_actionpolylinesegment.cpp
        actions/rs_actionprintpreview.cpp
        actions/rs_actionselect.cpp
        actions/rs_actionselectall.cpp
        actions/rs_actionselectbase.cpp
        actions/rs_actionselectcontour.cpp
        actions/rs_actionselectintersected.cpp
        actions/rs_actionselectinvert.cpp
        actions/rs_actionselectsingle.cpp
        actions/rs_actionselectwindow.cpp
        actions/rs_actionselectlayer.cpp
        actions/rs_actionsetrelativezero.cpp
        actions/rs_actionsetsnapmode.cpp
        actions/rs_actionsetsnaprestriction.cpp
        actions/rs_actionsnapintersectionmanual.cpp
        actions/rs_actiontoolregeneratedimensions.cpp
        actions/rs_actionzoomauto.cpp
        actions/rs_actionzoomautoy.cpp
        actions/rs_actionzoomin.cpp
        actions/rs_actionzoompan.cpp
        actions/rs_actionzoomprevious.cpp
        actions/rs_actionzoomredraw.cpp
        actions/rs_actionzoomscroll.cpp
        actions/rs_actionzoomwindow.cpp
        cmd/rs_commands.cpp
        plugins/intern/qc_actiongetpoint.cpp
        plugins/intern/qc_actiongetselect.cpp
        plugins/intern/qc_actiongetent.cpp
        main/lc_application.cpp
        main/qc_applicationwindow.cpp
        main/qc_dialogfactory.cpp
        main/qc_mdiwindow.cpp
        main/DocumentPluginImplementation.cpp
        main/mainwindowx.cpp
        main/ConvertLine.cpp
        main/ConvertLine.h
        main/PluginEntity.cpp
        main/PluginEntity.h
        main/console_dxf2pdf/console_dxf2pdf.cpp
        main/console_dxf2pdf/pdf_print_loop.cpp

        )
set(TEST_SOURCES
        test/test.cpp
        test/rs_creation_test.cpp)

set(TRANSLATION_DIR ${CMAKE_SOURCE_DIR}/librecad/ts)
set( TRANSLATIONS ${TRANSLATION_DIR}/librecad_ar.ts
    ${TRANSLATION_DIR}/librecad_ca.ts 
    ${TRANSLATION_DIR}/librecad_cs.ts 
    ${TRANSLATION_DIR}/librecad_en.ts 
    ${TRANSLATION_DIR}/librecad_en_au.ts 
    ${TRANSLATION_DIR}/librecad_da.ts 
    ${TRANSLATION_DIR}/librecad_de.ts 
    ${TRANSLATION_DIR}/librecad_el.ts 
    ${TRANSLATION_DIR}/librecad_es.ts 
    ${TRANSLATION_DIR}/librecad_es_ar.ts 
    ${TRANSLATION_DIR}/librecad_es_bo.ts 
    ${TRANSLATION_DIR}/librecad_es_cl.ts 
    ${TRANSLATION_DIR}/librecad_es_co.ts 
    ${TRANSLATION_DIR}/librecad_es_cr.ts 
    ${TRANSLATION_DIR}/librecad_es_do.ts 
    ${TRANSLATION_DIR}/librecad_es_ec.ts 
    ${TRANSLATION_DIR}/librecad_es_gt.ts 
    ${TRANSLATION_DIR}/librecad_es_hn.ts 
    ${TRANSLATION_DIR}/librecad_es_mx.ts 
    ${TRANSLATION_DIR}/librecad_es_ni.ts 
    ${TRANSLATION_DIR}/librecad_es_pa.ts 
    ${TRANSLATION_DIR}/librecad_es_pe.ts 
    ${TRANSLATION_DIR}/librecad_es_pr.ts 
    ${TRANSLATION_DIR}/librecad_es_py.ts 
    ${TRANSLATION_DIR}/librecad_es_sv.ts 
    ${TRANSLATION_DIR}/librecad_es_us.ts 
    ${TRANSLATION_DIR}/librecad_es_uy.ts 
    ${TRANSLATION_DIR}/librecad_es_ve.ts 
    ${TRANSLATION_DIR}/librecad_et.ts 
    ${TRANSLATION_DIR}/librecad_eu.ts 
    ${TRANSLATION_DIR}/librecad_fi.ts 
    ${TRANSLATION_DIR}/librecad_fr.ts 
    ${TRANSLATION_DIR}/librecad_gl.ts 
    ${TRANSLATION_DIR}/librecad_hi.ts 
    ${TRANSLATION_DIR}/librecad_hu.ts 
    ${TRANSLATION_DIR}/librecad_id_ID.ts 
    ${TRANSLATION_DIR}/librecad_it.ts 
    ${TRANSLATION_DIR}/librecad_ja.ts 
    ${TRANSLATION_DIR}/librecad_ko.ts 
    ${TRANSLATION_DIR}/librecad_lv.ts 
    ${TRANSLATION_DIR}/librecad_mk.ts 
    ${TRANSLATION_DIR}/librecad_nl.ts 
    ${TRANSLATION_DIR}/librecad_no.ts 
    ${TRANSLATION_DIR}/librecad_pa.ts 
    ${TRANSLATION_DIR}/librecad_pl.ts 
    ${TRANSLATION_DIR}/librecad_pt_br.ts 
    ${TRANSLATION_DIR}/librecad_pt_pt.ts 
    ${TRANSLATION_DIR}/librecad_ro_ro.ts 
    ${TRANSLATION_DIR}/librecad_ru.ts 
    ${TRANSLATION_DIR}/librecad_sk.ts 
    ${TRANSLATION_DIR}/librecad_sl.ts 
    ${TRANSLATION_DIR}/librecad_sq_al.ts 
    ${TRANSLATION_DIR}/librecad_sv.ts 
    ${TRANSLATION_DIR}/librecad_ta.ts 
    ${TRANSLATION_DIR}/librecad_tr.ts 
    ${TRANSLATION_DIR}/librecad_uk.ts 
    ${TRANSLATION_DIR}/librecad_zh_cn.ts 
    ${TRANSLATION_DIR}/librecad_zh_tw.ts
        )

if (${EMU_C99})
    set(SOURCES ${SOURCES} main/emu_c99.cpp)
endif ()

set( resources ${resources} ../res/extui/extui.qrc ../res/actions/actions.qrc ../res/icons/icons.qrc
        ../res/ui/ui.qrc
        ../res/main/main.qrc
        ../../licenses/licenses.qrc
        )

add_executable(librecad main/main.cpp ${SOURCES} ${resources})
add_executable(test_librecad ${TEST_SOURCES} ${SOURCES})
## Store intermedia stuff somewhere else
#GENERATED_DIR = ../../generated/librecad
# Use common project definitions.
#include(../../common.pri)
#include(./boost.pri)
#include(./muparser.pri)

include_directories(test_librecad
        ${GTEST_INCLUDE_DIRS}
        ../../libraries/libdxfrw/src
        ../../libraries/jwwlib/src
        ../../libraries/muparser/include
        cmd
        lib/actions
        lib/creation
        lib/debug
        lib/engine
        lib/fileio
        lib/filters
        lib/generators
        lib/gui
        lib/information
        lib/math
        lib/modification
        lib/printing
        actions
        main
        main/console_dxf2pdf
        test
        plugins
        ui
        ui/forms
        ui/generic
        ../res
        )

target_link_libraries(test_librecad
        PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::PrintSupport
        Qt5::Svg
        Qt5::Widgets
        dxfrw
        jwwlib
        muparser GTest::gtest GTest::gtest_main)

target_link_libraries(librecad
        PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::PrintSupport
        Qt5::Svg
        Qt5::Widgets
        dxfrw
        jwwlib
        muparser)

target_include_directories(librecad
        PRIVATE
        ../../libraries/libdxfrw/src
        ../../libraries/jwwlib/src
        ../../libraries/muparser/include
        cmd
        lib/actions
        lib/creation
        lib/debug
        lib/engine
        lib/fileio
        lib/filters
        lib/generators
        lib/gui
        lib/information
        lib/math
        lib/modification
        lib/printing
        actions
        main
        main/console_dxf2pdf
        test
        plugins
        ui
        ui/forms
        ui/generic
        ../res
)

foreach(source_translations ${TRANSLATIONS})
    if(EXISTS ${source_translations})
        cmake_path(GET source_translations STEM LAST_ONLY result)
        cmake_path(GET source_translations PARENT_PATH source_path)

        set(qm_file "${source_path}/${result}.qm")
        set(qm_files ${qm_files} ${qm_file})
    endif()
endforeach()
message(${qm_files})

set(SUPPORT_DIR ${CMAKE_SOURCE_DIR}/librecad/support)
add_custom_command(
        TARGET librecad POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SUPPORT_DIR}/patterns
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/patterns
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SUPPORT_DIR}/fonts
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/fonts
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SUPPORT_DIR}/library
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/library
        COMMAND ${TRANSLATOR} ${TRANSLATIONS}
        COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/qm
#        COMMAND ${CMAKE_COMMAND} -E copy
#            ${qm_files}
#            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/qm/
#        COMMAND ${CMAKE_COMMAND} -E remove
#            ${qm_files}
)

