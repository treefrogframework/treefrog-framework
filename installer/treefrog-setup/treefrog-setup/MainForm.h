#pragma once

#include <Windows.h>
#include "resource.h"

#undef GetTempPath

//
// リリースする際は50行目を編集！
//


namespace treefrogsetup {
    using namespace System;
    using namespace System::IO;
    using namespace System::Collections;
    using namespace System::Windows::Forms;
    using namespace System::Collections::Generic;
    using namespace System::Diagnostics;
    using namespace System::ComponentModel;

    // Gets Version String
    String^ VersionString()
    {
        return Reflection::Assembly::GetEntryAssembly()->GetName()->Version->ToString(3);
    }

    /// <summary>
    /// MainForm class
    /// </summary>
    public ref class MainForm : public System::Windows::Forms::Form
    {
    private: String^ msiName;
    private: System::Windows::Forms::Button^  browseButton;
    private: System::Windows::Forms::Button^  okButton;
    private: System::Windows::Forms::Button^  cancelButton;
    private: System::Windows::Forms::TextBox^  forderTextBox;
    private: System::Windows::Forms::Label^  labeltop;
    private: System::Windows::Forms::Label^  label;
    private: System::Windows::Forms::Label^  label1;
    private: System::Windows::Forms::PictureBox^  loadingImg;
    private: System::ComponentModel::BackgroundWorker^  bgWorker;

    private: 
        static initonly String^ TF_ENV_BAT = "C:\\TreeFrog\\" + VersionString() + "\\bin\\tfenv.bat";  // Base Directory

        //
        // バージョン
        //
        static initonly String^ VERSION_STR6_NEW  = L"6.2";
        static initonly String^ VERSION_STR6_PREV = L"6.1";

    public:
        MainForm(void)
        {
            InitializeComponent();
            this->loadingImg->Hide();

            // Background Worker
            bgWorker->DoWork += gcnew DoWorkEventHandler(this, &MainForm::bgWorker_DoWork);
            bgWorker->RunWorkerCompleted += gcnew RunWorkerCompletedEventHandler(this, &MainForm::bgWorker_RunWorkerCompleted);

            this->Text = "TreeFrog Framework " + VersionString() + " Setup";
            String^ folder = L"C:\\Qt";

            //try {
            //    // Check Qt5 Folder
            //    String^ fol = L"C:\\Qt";
            //    array<String^>^ dir = Directory::GetDirectories(fol);

            //    if (dir->Length > 0) {
            //        fol = dir[0];
            //        for (int i = 1; i < dir->Length; ++i) {
            //            if (dir[i]->IndexOf("creator", StringComparison::OrdinalIgnoreCase) < 0) {
            //                fol = (String::Compare(fol, dir[i]) >= 0) ? fol : dir[i];
            //            }
            //        }
            //        folder = fol;
            //    }
            //} catch (...) {
            //    //
            //}

            forderTextBox->Text = folder;
        }

    protected:
        /// <summary>
        /// Destructor
        /// </summary>
        ~MainForm()
        {
            if (components) {
                delete components;
            }
        }

    private:
        /// <summary>
        /// Variables for Designer
        /// </summary>
        System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
        /// <summary>
        /// デザイナー サポートに必要なメソッドです。このメソッドの内容を
        /// コード エディターで変更しないでください。
        /// </summary>
        void InitializeComponent(void)
        {
            System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MainForm::typeid));
            this->browseButton = (gcnew System::Windows::Forms::Button());
            this->okButton = (gcnew System::Windows::Forms::Button());
            this->cancelButton = (gcnew System::Windows::Forms::Button());
            this->forderTextBox = (gcnew System::Windows::Forms::TextBox());
            this->label = (gcnew System::Windows::Forms::Label());
            this->label1 = (gcnew System::Windows::Forms::Label());
            this->labeltop = (gcnew System::Windows::Forms::Label());
            this->loadingImg = (gcnew System::Windows::Forms::PictureBox());
            this->bgWorker = (gcnew System::ComponentModel::BackgroundWorker());
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->loadingImg))->BeginInit();
            this->SuspendLayout();
            // 
            // browseButton
            // 
            this->browseButton->Location = System::Drawing::Point(318, 103);
            this->browseButton->Name = L"browseButton";
            this->browseButton->Size = System::Drawing::Size(82, 24);
            this->browseButton->TabIndex = 0;
            this->browseButton->Text = L"Browse...";
            this->browseButton->UseVisualStyleBackColor = true;
            this->browseButton->Click += gcnew System::EventHandler(this, &MainForm::browseFolder);
            // 
            // okButton
            // 
            this->okButton->Location = System::Drawing::Point(227, 162);
            this->okButton->Name = L"okButton";
            this->okButton->Size = System::Drawing::Size(82, 24);
            this->okButton->TabIndex = 1;
            this->okButton->Text = L"Next";
            this->okButton->UseVisualStyleBackColor = true;
            this->okButton->Click += gcnew System::EventHandler(this, &MainForm::okButton_Click);
            // 
            // cancelButton
            // 
            this->cancelButton->Location = System::Drawing::Point(318, 162);
            this->cancelButton->Name = L"cancelButton";
            this->cancelButton->Size = System::Drawing::Size(82, 24);
            this->cancelButton->TabIndex = 2;
            this->cancelButton->Text = L"Cancel";
            this->cancelButton->UseVisualStyleBackColor = true;
            this->cancelButton->Click += gcnew System::EventHandler(this, &MainForm::cancelButton_Click);
            // 
            // forderTextBox
            // 
            this->forderTextBox->Location = System::Drawing::Point(35, 106);
            this->forderTextBox->Name = L"forderTextBox";
            this->forderTextBox->Size = System::Drawing::Size(274, 19);
            this->forderTextBox->TabIndex = 3;
            this->forderTextBox->TabStop = false;
            this->forderTextBox->Click += gcnew System::EventHandler(this, &MainForm::browseFolder);
            // 
            // label
            // 
            this->label->AutoSize = true;
            this->label->Font = (gcnew System::Drawing::Font(L"MS UI Gothic", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(128)));
            this->label->Location = System::Drawing::Point(32, 46);
            this->label->Name = L"label";
            this->label->Size = System::Drawing::Size(309, 15);
            this->label->TabIndex = 4;
            this->label->Text = L"Specify a base folder of Qt version " + VERSION_STR6_NEW + " or " + VERSION_STR6_PREV + ".";
            // 
            // label1
            // 
            this->label1->AutoSize = true;
            this->label1->Font = (gcnew System::Drawing::Font(L"MS UI Gothic", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(128)));
            this->label1->Location = System::Drawing::Point(43, 75);
            this->label1->Name = L"label1";
            this->label1->Size = System::Drawing::Size(162, 15);
            this->label1->TabIndex = 5;
            this->label1->Text = L"Example:  C:\\Qt\\" + VERSION_STR6_NEW + ".0\\msvc2019_64";
            // 
            // labeltop
            // 
            this->labeltop->AutoSize = true;
            this->labeltop->Font = (gcnew System::Drawing::Font(L"MS UI Gothic", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(128)));
            this->labeltop->Location = System::Drawing::Point(32, 18);
            this->labeltop->Name = L"labeltop";
            this->labeltop->Size = System::Drawing::Size(277, 15);
            this->labeltop->TabIndex = 6;
            this->labeltop->Text = L"TreeFrog Framework requires Qt.";
            // 
            // loadingImg
            // 
            this->loadingImg->BackColor = System::Drawing::SystemColors::Control;
            this->loadingImg->BackgroundImageLayout = System::Windows::Forms::ImageLayout::None;
            this->loadingImg->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"loadingImg.Image")));
            this->loadingImg->Location = System::Drawing::Point(185, 156);
            this->loadingImg->Name = L"loadingImg";
            this->loadingImg->Size = System::Drawing::Size(31, 34);
            this->loadingImg->TabIndex = 7;
            this->loadingImg->TabStop = false;
            // 
            // bgWorker
            // 
            this->bgWorker->DoWork += gcnew System::ComponentModel::DoWorkEventHandler(this, &MainForm::bgWorker_DoWork);
            // 
            // MainForm
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(417, 206);
            this->Controls->Add(this->loadingImg);
            this->Controls->Add(this->labeltop);
            this->Controls->Add(this->label1);
            this->Controls->Add(this->label);
            this->Controls->Add(this->forderTextBox);
            this->Controls->Add(this->cancelButton);
            this->Controls->Add(this->okButton);
            this->Controls->Add(this->browseButton);
            this->MaximizeBox = false;
            this->MinimizeBox = false;
            this->Name = L"MainForm";
            this->ShowIcon = false;
            this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
            this->Text = L"TreeFrog Framework SDK Setup";
            (cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->loadingImg))->EndInit();
            this->ResumeLayout(false);
            this->PerformLayout();

        }
#pragma endregion

    private:
        System::Void browseFolder(System::Object^  sender, System::EventArgs^  e)
        {
            // Folder Browser
            FolderBrowserDialog ^fbd = gcnew FolderBrowserDialog();
            fbd->SelectedPath = forderTextBox->Text;
            Windows::Forms::DialogResult res = fbd->ShowDialog();

            if ( res == Windows::Forms::DialogResult::OK ) {
                forderTextBox->Text = fbd->SelectedPath;
            }
        }

        //
        //
        //
        static List<String ^>^ searchSubDirectories(String^ name, String^ folderPath, array<String^> ^excludes)
        {
            List<String ^>^ ret = gcnew List<String^>();
            try {
                array<String^>^ dir = Directory::GetDirectories(folderPath, name);
                if (dir->Length > 0) {
                    return gcnew List<String^>(dir);
                }

                dir = Directory::GetDirectories(folderPath);
                for (int i = 0; i < dir->Length; ++i) {
                    // check excludes
                    bool exc = false;
                    for (int j = 0; j < excludes->Length; ++j) {
                        if (dir[i]->EndsWith(L"\\" + excludes[j], true, nullptr)) {
                            exc = true;
                            break;
                        }
                    }

                    if (exc) {
                        continue;
                    }

                    if (dir[i]->EndsWith(L"\\" + name)) {
                        ret->Add(dir[i]);
                    } else {
                        ret->AddRange(searchSubDirectories(name, dir[i], excludes));
                    }
                }

            } catch (Exception^) {
                //e->Message;
            }
            return ret;
        }

        static List<String ^>^ searchSubDirectories(String^ name, List<String ^>^ folderPaths, array<String^> ^excludes)
        {
            List<String ^>^ ret = gcnew List<String^>();
            for (int i = 0; i < folderPaths->Count; ++i) {
                ret->AddRange(searchSubDirectories(name, folderPaths[i], excludes));
            }
            return ret;
        }


        //
        //
        //
        static String^ searchFile(List<String ^>^ directories, String ^fileName)
        {
            try {
                for (int i = 0; i < directories->Count; ++i) {
                    array<String^>^ files = Directory::GetFiles(directories[i], fileName);
                    if (files->Length > 0) {
                        return files[0];
                    }
                }
            } catch (Exception^ e) {
                abort("Error Exception: " + e->Message, "Error");
            }
            return "";
        }


        // Abort
        private: static void abort(String^ text, String^ caption)
        {
            MessageBox::Show(text, caption, MessageBoxButtons::OK, MessageBoxIcon::Error);
            Environment::ExitCode = 1;
            Application::Exit();
        }

        private: System::Void okButton_Click(System::Object^ sender, System::EventArgs^ e)
        {
            this->okButton->Enabled = false;
            this->cancelButton->Enabled = false;
            this->browseButton->Enabled = false;
            this->forderTextBox->Enabled = false;
            this->loadingImg->Show();

            msiName = Path::GetTempPath() + Path::GetRandomFileName() + ".msi";
            bgWorker->RunWorkerAsync(msiName);
        }

        //
        //
        //
        private: System::Void bgWorker_DoWork(Object^ sender, DoWorkEventArgs^ e)
        {
            array<String^>^ excludes = { "Src", "QtCreator", "examples" };  // Folder to exclude
            List<String ^>^ bins = gcnew List<String ^>();

            if (forderTextBox->Text != L"C:\\") {
                //bins->AddRange(searchSubDirectories(L"bin", searchSubDirectories(L"mingw*", forderTextBox->Text, excludes), excludes));
                bins->AddRange(searchSubDirectories(L"bin", searchSubDirectories(L"msvc20*", forderTextBox->Text, excludes), excludes));

                // Qt 5.9 or later
                if (bins->Count == 0) {
                    bins->AddRange(searchSubDirectories(L"bin", forderTextBox->Text, excludes));
                }
            }

            if (bins->Count == 0) {
                abort("Not found Qt base folder.\n\nSetup aborts.", "Abort");
                return;
            }

            // Get Qt version
            String^ version;
            String^ file = searchFile(bins, "qmake.exe");
            if (file->Length > 0) {
                Process^ qmake = gcnew Diagnostics::Process;
                qmake->StartInfo->FileName = file;
                qmake->StartInfo->Arguments = "-v";
                qmake->StartInfo->CreateNoWindow = true;
                qmake->StartInfo->WindowStyle = ProcessWindowStyle::Minimized;
                qmake->StartInfo->RedirectStandardOutput = true;
                qmake->StartInfo->UseShellExecute = false;
                qmake->Start();
                version = qmake->StandardOutput->ReadToEnd();
                qmake->WaitForExit();
            } else {
                abort("Not found qmake.exe.\n\nSetup aborts.", "Abort");
                return;
            }

            // Gets path of qtenv2.bat
            String^ qtenv = searchFile(bins, "qtenv2.bat");
            if (qtenv->Length == 0) {
                abort("Not found qtenv2.bat.\n\nSetup aborts.", "Abort");
                return;
            } else {
                e->Result = qtenv;
            }

            // Get msi file from resource
            int rcid = 0;
            if (version->IndexOf("Qt version " + VERSION_STR6_NEW, StringComparison::OrdinalIgnoreCase) > 0) {
                rcid = IDR_TREEFROG_QT602_MSI;
            } else if (version->IndexOf("Qt version " + VERSION_STR6_PREV, StringComparison::OrdinalIgnoreCase) > 0) {
                rcid = IDR_TREEFROG_QT601_MSI;
            } else {
                abort("Not found Qt version " + VERSION_STR6_NEW + " or " + VERSION_STR6_PREV + ".", "Abort");
                return;
            }

            System::Reflection::Module^ mod = System::Reflection::Assembly::GetExecutingAssembly()->GetModules()[0];
            HINSTANCE hInst = static_cast<HINSTANCE>(System::Runtime::InteropServices::Marshal::GetHINSTANCE(mod).ToPointer());
            HRSRC hRsrc = FindResource(hInst, MAKEINTRESOURCE(rcid), L"TREEFROG_MSI");
            DWORD lenRes = SizeofResource(hInst, hRsrc);
            HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
            byte *pRes = (byte *)LockResource(hGlobal);

            FileStream^ fs = gcnew FileStream(msiName, FileMode::Create);
            BinaryWriter^ writer = gcnew BinaryWriter(fs);

            try {
                // Writes msi
                byte *pEnd = pRes + lenRes;
                while (pRes < pEnd) {
                    writer->Write(*pRes++);
                }

                //// Result string
                //String^ qtbin;
                //for (int i = 0; i < bins->Count; ++i) {
                //    qtbin += bins[i] + ";";
                //}
                //e->Result = qtbin;

            } catch (Exception^ e) {
                abort("Error Exception: " + e->Message, "Error");
            }

            fs->Close();
        }

        //
        //
        //
        private: System::Void bgWorker_RunWorkerCompleted(Object^ sender, RunWorkerCompletedEventArgs^ e)
        {
            try {
                this->Hide();

                String^ qtenv = e->Result->ToString();
                if (qtenv->Length == 0) {
                    abort("Not found Qt base folder.\n\nSetup aborts.", "Abort");
                    return;
                }

                Process^ proc = (gcnew Diagnostics::Process())->Start(msiName);
                proc->WaitForExit();

                if (proc->ExitCode != 0) {
                    Application::Exit();
                    return;
                }

                //// Edits tfenv.bat
                //IO::FileInfo^ fibat = gcnew IO::FileInfo(TF_ENV_BAT);
                //if (proc->ExitCode == 0 && fibat->Exists) {
                //    String^ out;

                //    StreamReader^ din = File::OpenText(TF_ENV_BAT);
                //    String^ line;
                //    while ((line = din->ReadLine()) != nullptr) {
                //        if (line->StartsWith("set PATH=")) {
                //            line = L"set PATH=%TFDIR%\\bin;" + qtbin + "%PATH%";
                //        }
                //        out += line + "\r\n";
                //    }
                //    din->Close();

                //    StreamWriter^ dout = gcnew StreamWriter(TF_ENV_BAT);
                //    dout->Write(out);
                //    dout->Close();
                //}

                // Edits tfenv.bat
                IO::FileInfo^ fibat = gcnew IO::FileInfo(TF_ENV_BAT);
                if (proc->ExitCode == 0 && fibat->Exists) {
                    String^ out;

                    StreamReader^ din = File::OpenText(TF_ENV_BAT);
                    String^ line;
                    while ((line = din->ReadLine()) != nullptr) {
                        if (line->StartsWith("set QTENV=")) {
                            line = L"set QTENV=\"" + qtenv + "\"";
                        }
                        out += line + "\r\n";
                    }
                    din->Close();

                    StreamWriter^ dout = gcnew StreamWriter(TF_ENV_BAT);
                    dout->Write(out);
                    dout->Close();
                }

            } catch (Exception^ e) {
                abort("Error Exception: " + e->Message, "Error");
            }

            try {
                // Cleanup
                IO::FileInfo^ fi = gcnew IO::FileInfo(msiName);
                if (fi->Exists) {
                    fi->Delete();
                }
            } catch (...) {
            }
            Application::Exit();
        }


        private: System::Void cancelButton_Click(System::Object^  sender, System::EventArgs^  e)
        {
            Application::Exit();
        }
};
}
