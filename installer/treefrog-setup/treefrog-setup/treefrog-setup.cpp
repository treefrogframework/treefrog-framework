// treefrog-setup.cpp : メイン プロジェクト ファイルです。

#include "stdafx.h"
#include "MainForm.h"

using namespace treefrogsetup;


[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false); 

    String^ msg = "Welcome to the TreeFrog Framework " + VersionString() + " Setup Wizard.\n\nTreeFrog Framework will be installed in the \"C:\\TreeFrog\\\" folder.\n\n Are you sure?";
    String^ caption = "TreeFrog Framework " + VersionString() + " Setup";

    DialogResult result = MessageBox::Show(msg, caption, MessageBoxButtons::YesNo, MessageBoxIcon::Information);
    if (result != DialogResult::Yes) {
        return 1;
    }

    // Main Form
    Application::Run(gcnew MainForm());
    return 0;
}
