// treefrog-setup.cpp : メイン プロジェクト ファイルです。

#include "stdafx.h"
#include "MainForm.h"

using namespace treefrogsetup;


[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false); 

    DialogResult result = MessageBox::Show("TreeFrog Framework SDK will be installed in the \"C:\\TreeFrog\\\" folder.\n\n  Are you sure?", "TreeFrog Framework SDK Setup", MessageBoxButtons::YesNo);
    if (result != DialogResult::Yes) {
        return 1;
    }

    // Main Form
    Application::Run(gcnew MainForm());
    return 0;
}
