#pragma once
// A lóxica de Barullo Xestor, sen interface: descubrir os plugins, ler o instalado, descargar,
// instalar, desinstalar e pedir o permiso de administrador cando fai falla.
//
//   Version        versións «0.4.1» e comparación
//   Produto        produto.env, produtos, formatos, versións publicadas
//   Rutas          onde van os plugins en Mac, Windows e Linux (usuario e sistema)
//   Escaner        que hai instalado e en que versión
//   Estado         ao día, actualización, copia vella no sistema...
//   Rede, GitHub   a API e raw.githubusercontent.com, con caché e ETag
//   Descubrimento  axustes e catálogo (GitHub + repos locais)
//   Papeleira      o Lixo de cada sistema
//   Admin          o permiso de administrador (osascript, UAC, pkexec)
//   Instalador     instalar, actualizar e desinstalar
#include "Version.h"
#include "Produto.h"
#include "Rutas.h"
#include "Escaner.h"
#include "Estado.h"
#include "Rede.h"
#include "GitHub.h"
#include "Descubrimento.h"
#include "Proceso.h"
#include "Papeleira.h"
#include "Admin.h"
#include "Instalador.h"
