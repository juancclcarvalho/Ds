//[]---------------------------------------------------------------[]
//|                                                                 |
//| Copyright (C) 2022 Paulo Pagliosa.                              |
//|                                                                 |
//| This software is provided 'as-is', without any express or       |
//| implied warranty. In no event will the authors be held liable   |
//| for any damages arising from the use of this software.          |
//|                                                                 |
//| Permission is granted to anyone to use this software for any    |
//| purpose, including commercial applications, and to alter it and |
//| redistribute it freely, subject to the following restrictions:  |
//|                                                                 |
//| 1. The origin of this software must not be misrepresented; you  |
//| must not claim that you wrote the original software. If you use |
//| this software in a product, an acknowledgment in the product    |
//| documentation would be appreciated but is not required.         |
//|                                                                 |
//| 2. Altered source versions must be plainly marked as such, and  |
//| must not be misrepresented as being the original software.      |
//|                                                                 |
//| 3. This notice may not be removed or altered from any source    |
//| distribution.                                                   |
//|                                                                 |
//[]---------------------------------------------------------------[]
//
// OVERVIEW: BoxSource.cpp
// ========
// Source file for vis box source.
//
// Author: Paulo Pagliosa
// Last revision: 08/03/2022

#include "geometry/MeshSweeper.h"
#include "BoxSource.h"

namespace cg::vis
{ // begin namespace cg::vis


/////////////////////////////////////////////////////////////////////
//
// BoxSource implementation
// =========
void
BoxSource::execute()
{
  setOutput(new TriCellMesh{*MeshSweeper::makeBox()});
#ifdef _DEBUG
  puts("BOX SOURCE EXECUTED");
#endif // _DEBUG
}

} // end namespace cg::vis
