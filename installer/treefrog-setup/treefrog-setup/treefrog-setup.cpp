// treefrog-setup.cpp : メイン プロジェクト ファイルです。

#include "stdafx.h"
#include "MainForm.h"

using namespace treefrogsetup;



[STAThreadAttribute]
int main(array<System::String ^> ^args)
{
	// コントロールが作成される前に、Windows XP ビジュアル効果を有効にします
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	//Thread::CurrentThread->CurrentCulture = gcnew System::Globalization::CultureInfo("en-US");
//	System::Globalization::CultureInfo("en-US");
	//Thread::CurrentThread::CurrentCulture = gcnew CultureInfo("en-US");

	// メイン ウィンドウを作成して、実行します
	Application::Run(gcnew MainForm());


	//OpenFileDialog ^openFileDialog1 = gcnew OpenFileDialog();
    //openFileDialog1->Filter = "Cursor Files|*.cur";
    //openFileDialog1->Title = "Select a Cursor File";


	return 0;
}
