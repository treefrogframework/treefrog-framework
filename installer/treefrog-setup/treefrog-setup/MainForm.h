#pragma once

#include <Windows.h>
#include "resource.h"

#undef GetTempPath


namespace treefrogsetup {

    using namespace System;
    using namespace System::IO;
    using namespace System::Collections;
    using namespace System::Windows::Forms;
    using namespace System::Collections::Generic;
    using namespace System::Diagnostics;

    /// <summary>
    /// MainForm class
    /// </summary>
    public ref class MainForm : public System::Windows::Forms::Form
    {
    public:
        /*
         * TreeFrog Framework Base Directory
         */
        static initonly String^ TF_VERSION = "1.6.1";
        static initonly String^ TF_ENV_BAT = "C:\\TreeFrog\\" + TF_VERSION + " \\bin\\tfenv.bat";

        MainForm(void)
        {
            InitializeComponent();

            // Check Qt5 Folder
            String^ folder = L"C:\\Qt";
            array<String^>^ dir = Directory::GetDirectories(folder);

            if (dir->Length > 0) {
                folder = dir[0];
                for (int i = 1; i < dir->Length; ++i)
                    folder = (String::Compare(folder, dir[i]) >= 0) ? folder : dir[i];
            } else {
                folder = L"C:\\";
            }

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

    private: System::Windows::Forms::Button^  browseButton;
    private: System::Windows::Forms::Button^  okButton;
    private: System::Windows::Forms::Button^  cancelButton;
    private: System::Windows::Forms::TextBox^  forderTextBox;
    private: System::Windows::Forms::Label^  label;
    private: System::Windows::Forms::Label^  label1;

    protected: 

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
            this->browseButton = (gcnew System::Windows::Forms::Button());
            this->okButton = (gcnew System::Windows::Forms::Button());
            this->cancelButton = (gcnew System::Windows::Forms::Button());
            this->forderTextBox = (gcnew System::Windows::Forms::TextBox());
            this->label = (gcnew System::Windows::Forms::Label());
            this->label1 = (gcnew System::Windows::Forms::Label());
            this->SuspendLayout();
            // 
            // browseButton
            // 
            this->browseButton->Location = System::Drawing::Point(398, 83);
            this->browseButton->Name = L"browseButton";
            this->browseButton->Size = System::Drawing::Size(109, 27);
            this->browseButton->TabIndex = 0;
            this->browseButton->Text = L"Browse...";
            this->browseButton->UseVisualStyleBackColor = true;
            this->browseButton->Click += gcnew System::EventHandler(this, &MainForm::browseFolder);
            // 
            // okButton
            // 
            this->okButton->Location = System::Drawing::Point(254, 137);
            this->okButton->Name = L"okButton";
            this->okButton->Size = System::Drawing::Size(115, 30);
            this->okButton->TabIndex = 1;
            this->okButton->Text = L"Continue";
            this->okButton->UseVisualStyleBackColor = true;
            this->okButton->Click += gcnew System::EventHandler(this, &MainForm::okButton_Click);
            // 
            // cancelButton
            // 
            this->cancelButton->Location = System::Drawing::Point(398, 137);
            this->cancelButton->Name = L"cancelButton";
            this->cancelButton->Size = System::Drawing::Size(109, 30);
            this->cancelButton->TabIndex = 2;
            this->cancelButton->Text = L"Cancel";
            this->cancelButton->UseVisualStyleBackColor = true;
            this->cancelButton->Click += gcnew System::EventHandler(this, &MainForm::cancelButton_Click);
            // 
            // forderTextBox
            // 
            this->forderTextBox->Location = System::Drawing::Point(34, 87);
            this->forderTextBox->Name = L"forderTextBox";
            this->forderTextBox->Size = System::Drawing::Size(335, 19);
            this->forderTextBox->TabIndex = 3;
            this->forderTextBox->Click += gcnew System::EventHandler(this, &MainForm::browseFolder);
            // 
            // label
            // 
            this->label->AutoSize = true;
            this->label->Font = (gcnew System::Drawing::Font(L"MS UI Gothic", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(128)));
            this->label->Location = System::Drawing::Point(32, 23);
            this->label->Name = L"label";
            this->label->Size = System::Drawing::Size(261, 15);
            this->label->TabIndex = 4;
            this->label->Text = L"Specify the base folder of Qt version 5.";
            // 
            // label1
            // 
            this->label1->AutoSize = true;
            this->label1->Font = (gcnew System::Drawing::Font(L"MS UI Gothic", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
                static_cast<System::Byte>(128)));
            this->label1->Location = System::Drawing::Point(41, 50);
            this->label1->Name = L"label1";
            this->label1->Size = System::Drawing::Size(162, 15);
            this->label1->TabIndex = 5;
            this->label1->Text = L"Example:  C:\\Qt\\Qt5.0.2";
            // 
            // MainForm
            // 
            this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
            this->ClientSize = System::Drawing::Size(526, 192);
            this->Controls->Add(this->label1);
            this->Controls->Add(this->label);
            this->Controls->Add(this->forderTextBox);
            this->Controls->Add(this->cancelButton);
            this->Controls->Add(this->okButton);
            this->Controls->Add(this->browseButton);
            this->MaximizeBox = false;
            this->MinimizeBox = false;
            this->Name = L"MainForm";
            this->Text = L"TreeFrog Framework SDK Setup";
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

        static List<String ^>^ searchSubDirectories(const String^ name, array<String^> ^excludes, String^ folderPath)
        {
            List<String ^>^ ret = gcnew List<String^>();
            try {
                array<String^>^ dir = Directory::GetDirectories(folderPath);

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
                        ret->AddRange(searchSubDirectories(name, excludes, dir[i]));
                    }
                }

            } catch (Exception^) {
                //e->Message;
            }
            return ret;
        }

        private: static void abort(String^ text, String^ caption)
        {
            MessageBox::Show(text, caption, MessageBoxButtons::OK, MessageBoxIcon::Error);
            Environment::ExitCode = 1;
            Application::Exit();
        }

        private: System::Void okButton_Click(System::Object^ sender, System::EventArgs^ e)
        {
            array<String^>^ excludes = { "Src", "QtCreator" };  // Folder to exclude
            List<String ^>^ bins = gcnew List<String ^>();

            if (forderTextBox->Text != L"C:\\") {
                bins->AddRange(searchSubDirectories(L"bin", excludes, forderTextBox->Text));
            }

            if (bins->Count == 0) {
                abort("Not found Qt base folder.\n\nSetup aborts.", "Abort");
                return;
            }

            // Get msi file from resource
            System::Reflection::Module^ mod = System::Reflection::Assembly::GetExecutingAssembly()->GetModules()[0];
            HINSTANCE hInst = static_cast<HINSTANCE>(System::Runtime::InteropServices::Marshal::GetHINSTANCE(mod).ToPointer());
            HRSRC hRsrc = FindResource(hInst, MAKEINTRESOURCE(IDR_TREEFROG_MSI), L"TREEFROG_MSI");
            DWORD lenRes = SizeofResource(hInst, hRsrc);
            HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
            byte *pRes = (byte *)LockResource(hGlobal);

            String^ msiName = Path::GetTempPath() + Path::GetRandomFileName() + ".msi";
            FileStream^ fs = gcnew FileStream(msiName, FileMode::Create);
            BinaryWriter^ writer = gcnew BinaryWriter(fs);

            try {
                // Writes msi
                byte *pEnd = pRes + lenRes;
                while (pRes < pEnd) {
                    writer->Write(*pRes++);
                }
                fs->Close();
                this->Hide();

                // Starts msi
                Process^ proc = (gcnew Diagnostics::Process())->Start(msiName);
                proc->WaitForExit();

                // Edits tfenv.bat
                IO::FileInfo^ fibat = gcnew IO::FileInfo(TF_ENV_BAT);
                if (fibat->Exists) {
                    String^ qtbin;

                    for (int i = 0; i < bins->Count; ++i) {
                        qtbin += bins[i] + ";";
                    }

                    String^ out;

                    StreamReader^ din = File::OpenText(TF_ENV_BAT);
                    String^ line;
                    while ((line = din->ReadLine()) != nullptr) {
                        if (line->StartsWith("set PATH=")) {
                            line = L"set PATH=%TFDIR%\\bin;" + qtbin + "%PATH%";
                        }
                        out += line + "\r\n";
                    }
                    din->Close();

                    StreamWriter^ dout = gcnew StreamWriter(TF_ENV_BAT);
                    dout->Write(out);
                    dout->Close();
                }

            } catch (Exception^ e) {
                abort("File write error: " + e->Message, "Error");
            }

            fs->Close();
            IO::FileInfo^ fi = gcnew IO::FileInfo(msiName);
            fi->Delete();
            Application::Exit();
        }

        private: System::Void cancelButton_Click(System::Object^  sender, System::EventArgs^  e)
        {
            Application::Exit();
        }
};
}
