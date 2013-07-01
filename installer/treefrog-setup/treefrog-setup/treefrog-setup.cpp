// treefrog-setup.cpp : メイン プロジェクト ファイルです。

#include "stdafx.h"
#include "MainForm.h"

using namespace treefrogsetup;


[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false); 

    // Main Form
    Application::Run(gcnew MainForm());
    return 0;
}
