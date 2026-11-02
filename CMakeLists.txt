cmake_minimum_required(VERSION 3.22)

# Barullo Xestor: a app (JUCE) que ten ao día os plugins Barullo en Mac, Windows e Linux.
# En Mac: binario universal (Apple Silicon + Intel), desde macOS 11. En Linux/Windows non fai nada.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo")
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas de Mac")
endif()

project(BarulloXestor VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  enable_language(OBJCXX)
endif()

# JUCE: cartafol irmán ../JUCE, ou o que se indique con -DJUCE_DIR=... (coma nos plugins)
if(NOT JUCE_DIR)
  set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE")
endif()
get_filename_component(JUCE_DIR "${JUCE_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(EXISTS "${JUCE_DIR}/CMakeLists.txt")
  add_subdirectory("${JUCE_DIR}" JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(JUCE)
endif()

# Núcleo común de Escoita: a versión fixada ou, se existe, a carpeta irmá ../escoita-nucleo
# (o texto en galego e o aviso das versións, coma nos plugins); tamén -DESCOITA_NUCLEO=<carpeta>
set(ESCOITA_NUCLEO_VERSION "v0.5.0")
if(NOT ESCOITA_NUCLEO AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../escoita-nucleo/escoita_nucleo")
  set(ESCOITA_NUCLEO "${CMAKE_CURRENT_SOURCE_DIR}/../escoita-nucleo")
endif()
if(ESCOITA_NUCLEO)
  add_subdirectory("${ESCOITA_NUCLEO}" escoita-nucleo)
else()
  include(FetchContent)
  FetchContent_Declare(escoita_nucleo GIT_REPOSITORY https://github.com/callejodev-dotcom/escoita-nucleo.git
                       GIT_TAG ${ESCOITA_NUCLEO_VERSION} GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(escoita_nucleo)
endif()

# as tipografías da familia (Montserrat e Inter, licenza OFL)
juce_add_binary_data(XestorFontes SOURCES
  Resources/Inter-Medium.ttf
  Resources/Montserrat-SemiBold.ttf
  Resources/Montserrat-Bold.ttf)

# a lóxica (sen interface): descubrir, ler versións, descargar, instalar, desinstalar e pedir admin
set(NUCLEO_SOURCES
  Source/Nucleo/Produto.cpp
  Source/Nucleo/Rutas.cpp
  Source/Nucleo/Escaner.cpp
  Source/Nucleo/Estado.cpp
  Source/Nucleo/Rede.cpp
  Source/Nucleo/GitHub.cpp
  Source/Nucleo/Descubrimento.cpp
  Source/Nucleo/Proceso.cpp
  Source/Nucleo/Papeleira.cpp
  Source/Nucleo/Admin.cpp
  Source/Nucleo/Instalador.cpp
  Source/Nucleo/Copias.cpp
  Source/Nucleo/Sistema.cpp
  Source/Nucleo/Autoactualizacion.cpp
  Source/Nucleo/Diagnostico.cpp
  Source/Nucleo/Texto.cpp
  Source/Nucleo/Filtro.cpp
  Source/Nucleo/Traducions.cpp
  Source/App/CLI.cpp)
if(APPLE)
  list(APPEND NUCLEO_SOURCES Source/Nucleo/Papeleira_mac.mm)
endif()

set(APP_SOURCES
  Source/App/Main.cpp
  Source/App/Aspecto.cpp
  Source/App/Modelo.cpp
  Source/App/Vistas.cpp
  Source/App/Paneis.cpp
  Source/App/Bandeja.cpp)

set(XESTOR_DEFS
  JUCE_WEB_BROWSER=0
  JUCE_DISPLAY_SPLASH_SCREEN=0
  JUCE_USE_CURL=$<IF:$<PLATFORM_ID:Linux>,1,0>
  JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
  JUCE_APPLICATION_NAME_STRING="Barullo Xestor"
  JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}")

juce_add_gui_app(BarulloXestor
  PRODUCT_NAME "Barullo Xestor"
  COMPANY_NAME "Escoita"
  COMPANY_WEBSITE "https://www.escoita.link"
  BUNDLE_ID "link.escoita.barulloxestor"
  ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Icona.png"
  VERSION ${PROJECT_VERSION}
  HARDENED_RUNTIME_ENABLED FALSE)

target_sources(BarulloXestor PRIVATE ${NUCLEO_SOURCES} ${APP_SOURCES})
target_include_directories(BarulloXestor PRIVATE Source)
target_compile_definitions(BarulloXestor PRIVATE ${XESTOR_DEFS})
target_link_libraries(BarulloXestor
  PRIVATE
    XestorFontes
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_cryptography
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
escoita_produto(BarulloXestor NOME "Barullo Xestor" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/barulloxestor)
if(MSVC)
  target_compile_options(BarulloXestor PRIVATE /utf-8 /bigobj)
endif()
if(UNIX AND NOT APPLE)
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(CURL libcurl)
    if(CURL_FOUND)
      target_include_directories(BarulloXestor PRIVATE ${CURL_INCLUDE_DIRS})
    endif()
  endif()
endif()

# Avisos como erros no código propio (non no de JUCE)
option(BARULLO_WERROR "Avisos como erros en Source/ e probas/" OFF)
if(BARULLO_WERROR)
  if(MSVC)
    set_source_files_properties(${NUCLEO_SOURCES} ${APP_SOURCES} probas/Probas.cpp PROPERTIES COMPILE_OPTIONS "/WX")
  else()
    set_source_files_properties(${NUCLEO_SOURCES} ${APP_SOURCES} probas/Probas.cpp PROPERTIES COMPILE_OPTIONS "-Werror")
  endif()
endif()

# As probas: o núcleo e a liña de ordes en cartafoles temporais, cun GitHub de mentira (sen rede)
option(BARULLO_TESTS "Compilar as probas" ON)
if(BARULLO_TESTS)
  juce_add_console_app(XestorProbas PRODUCT_NAME "XestorProbas")
  target_sources(XestorProbas PRIVATE probas/Probas.cpp ${NUCLEO_SOURCES})
  target_include_directories(XestorProbas PRIVATE Source)
  target_compile_definitions(XestorProbas PRIVATE ${XESTOR_DEFS} XESTOR_PROBAS=1)
  target_link_libraries(XestorProbas
    PRIVATE juce::juce_gui_basics juce::juce_cryptography
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
  escoita_produto(XestorProbas NOME "Barullo Xestor" VERSION ${PROJECT_VERSION} REPO callejodev-dotcom/barulloxestor PROBA)
  if(MSVC)
    target_compile_options(XestorProbas PRIVATE /utf-8 /bigobj)
  endif()
endif()
