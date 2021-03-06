/*
 * PamScriptDialog.cpp
 *
 *  Created on: 18/01/2012
 *      Author: u07286
 */

#include "PamScriptDialog.h"


PamScriptDialog::PamScriptDialog(PamHandler *pamh)
{
	this->pamh = pamh;
}

PamScriptDialog::~PamScriptDialog() {
}

void PamScriptDialog::alert(const char *msg)
{
	pamh->notify(msg);
}



void PamScriptDialog::progressMessage(const char *msg)
{
	pamh->notify (msg);
}



void PamScriptDialog::cancelProgressMessage()
{
}


std::wstring PamScriptDialog::selectAccount(std::vector<std::wstring>accounts)
{
	return std::wstring();
}
