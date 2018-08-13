#pragma once

#include "resource.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace MainPath {

	/// <summary>
	/// Form2 的摘要
	///
	/// 警告: 如果您變更這個類別的名稱，就必須變更與這個類別所依據之所有 .resx 檔案關聯的
	///          Managed 資源編譯器工具的 'Resource File Name' 屬性。
	///          否則，這些設計工具
	///          將無法與這個表單關聯的當地語系化資源
	///          正確互動。
	/// </summary>
	public ref class Form2 : public System::Windows::Forms::Form
	{
	private: System::Windows::Forms::Button^  button1;


	private: System::Windows::Forms::TextBox^  textBox11;
	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::ComboBox^  comboBox6;
	private: System::Windows::Forms::ComboBox^  comboBox7;
	private: System::Windows::Forms::ComboBox^  comboBox8;
	private: System::Windows::Forms::ComboBox^  comboBox9;
	private: System::Windows::Forms::ComboBox^  comboBox10;

	private: System::Windows::Forms::Button^  button3;
	public:
		Form2(void)
		{
			InitializeComponent();
			//
			//TODO: 在此加入建構函式程式碼
			//
		}
		// following are parameters for clan analysis
		String ^clanspec;	
		String ^ccode1;
		String ^ccode2;
		String ^ccode3;
		String ^ccode4;
		String ^ccode5;
		String ^bratio1;
		String ^bratio2;
		String ^bratio3;
		String ^bratio4;
		String ^bratio5;
		int ctype1;	// for patents
		int ctype2;
		int ctype3;
		int ctype4;
		int ctype5;
		int ptype1;	// for papers
		int ptype2;
		int ptype3;
		int ptype4;
		int ptype5;
		int ftype;	// WOS or patent data, etc.
		int exit_type;

	protected:
		/// <summary>
		/// 清除任何使用中的資源。
		/// </summary>
		~Form2()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TextBox^  textBox1;
	protected: 
	private: System::Windows::Forms::TextBox^  textBox2;
	private: System::Windows::Forms::TextBox^  textBox3;
	private: System::Windows::Forms::TextBox^  textBox4;
	private: System::Windows::Forms::TextBox^  textBox5;
	private: System::Windows::Forms::TextBox^  textBox6;
	private: System::Windows::Forms::TextBox^  textBox7;
	private: System::Windows::Forms::TextBox^  textBox8;
	private: System::Windows::Forms::TextBox^  textBox9;
	private: System::Windows::Forms::TextBox^  textBox10;
	private: System::Windows::Forms::ComboBox^  comboBox1;
	private: System::Windows::Forms::ComboBox^  comboBox2;
	private: System::Windows::Forms::ComboBox^  comboBox3;
	private: System::Windows::Forms::ComboBox^  comboBox4;
	private: System::Windows::Forms::ComboBox^  comboBox5;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;

	private:
		/// <summary>
		/// 設計工具所需的變數。
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// 此為設計工具支援所需的方法 - 請勿使用程式碼編輯器修改這個方法的內容。
		///
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form2::typeid));
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->textBox2 = (gcnew System::Windows::Forms::TextBox());
			this->textBox3 = (gcnew System::Windows::Forms::TextBox());
			this->textBox4 = (gcnew System::Windows::Forms::TextBox());
			this->textBox5 = (gcnew System::Windows::Forms::TextBox());
			this->textBox6 = (gcnew System::Windows::Forms::TextBox());
			this->textBox7 = (gcnew System::Windows::Forms::TextBox());
			this->textBox8 = (gcnew System::Windows::Forms::TextBox());
			this->textBox9 = (gcnew System::Windows::Forms::TextBox());
			this->textBox10 = (gcnew System::Windows::Forms::TextBox());
			this->comboBox1 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox2 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox3 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox4 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox5 = (gcnew System::Windows::Forms::ComboBox());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->textBox11 = (gcnew System::Windows::Forms::TextBox());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->comboBox6 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox7 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox8 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox9 = (gcnew System::Windows::Forms::ComboBox());
			this->comboBox10 = (gcnew System::Windows::Forms::ComboBox());
			this->SuspendLayout();
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(31, 83);
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(230, 22);
			this->textBox1->TabIndex = 0;
			this->textBox1->TextChanged += gcnew System::EventHandler(this, &Form2::textBox1_TextChanged);
			// 
			// textBox2
			// 
			this->textBox2->Location = System::Drawing::Point(31, 121);
			this->textBox2->Name = L"textBox2";
			this->textBox2->Size = System::Drawing::Size(230, 22);
			this->textBox2->TabIndex = 1;
			this->textBox2->TextChanged += gcnew System::EventHandler(this, &Form2::textBox2_TextChanged);
			// 
			// textBox3
			// 
			this->textBox3->Location = System::Drawing::Point(31, 159);
			this->textBox3->Name = L"textBox3";
			this->textBox3->Size = System::Drawing::Size(230, 22);
			this->textBox3->TabIndex = 2;
			this->textBox3->TextChanged += gcnew System::EventHandler(this, &Form2::textBox3_TextChanged);
			// 
			// textBox4
			// 
			this->textBox4->Location = System::Drawing::Point(31, 199);
			this->textBox4->Name = L"textBox4";
			this->textBox4->Size = System::Drawing::Size(230, 22);
			this->textBox4->TabIndex = 3;
			this->textBox4->TextChanged += gcnew System::EventHandler(this, &Form2::textBox4_TextChanged);
			// 
			// textBox5
			// 
			this->textBox5->Location = System::Drawing::Point(31, 238);
			this->textBox5->Name = L"textBox5";
			this->textBox5->Size = System::Drawing::Size(230, 22);
			this->textBox5->TabIndex = 4;
			this->textBox5->TextChanged += gcnew System::EventHandler(this, &Form2::textBox5_TextChanged);
			// 
			// textBox6
			// 
			this->textBox6->Location = System::Drawing::Point(296, 83);
			this->textBox6->Name = L"textBox6";
			this->textBox6->Size = System::Drawing::Size(68, 22);
			this->textBox6->TabIndex = 5;
			this->textBox6->TextChanged += gcnew System::EventHandler(this, &Form2::textBox6_TextChanged);
			// 
			// textBox7
			// 
			this->textBox7->Location = System::Drawing::Point(296, 121);
			this->textBox7->Name = L"textBox7";
			this->textBox7->Size = System::Drawing::Size(68, 22);
			this->textBox7->TabIndex = 6;
			this->textBox7->TextChanged += gcnew System::EventHandler(this, &Form2::textBox7_TextChanged);
			// 
			// textBox8
			// 
			this->textBox8->Location = System::Drawing::Point(296, 159);
			this->textBox8->Name = L"textBox8";
			this->textBox8->Size = System::Drawing::Size(68, 22);
			this->textBox8->TabIndex = 7;
			this->textBox8->TextChanged += gcnew System::EventHandler(this, &Form2::textBox8_TextChanged);
			// 
			// textBox9
			// 
			this->textBox9->Location = System::Drawing::Point(296, 199);
			this->textBox9->Name = L"textBox9";
			this->textBox9->Size = System::Drawing::Size(68, 22);
			this->textBox9->TabIndex = 8;
			this->textBox9->TextChanged += gcnew System::EventHandler(this, &Form2::textBox9_TextChanged);
			// 
			// textBox10
			// 
			this->textBox10->Location = System::Drawing::Point(296, 238);
			this->textBox10->Name = L"textBox10";
			this->textBox10->Size = System::Drawing::Size(68, 22);
			this->textBox10->TabIndex = 9;
			this->textBox10->TextChanged += gcnew System::EventHandler(this, &Form2::textBox10_TextChanged);
			// 
			// comboBox1
			// 
			this->comboBox1->FormattingEnabled = true;
			this->comboBox1->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"IPC", L"UPC", L"Assignee", L"Country"});
			this->comboBox1->Location = System::Drawing::Point(399, 83);
			this->comboBox1->Name = L"comboBox1";
			this->comboBox1->Size = System::Drawing::Size(92, 20);
			this->comboBox1->TabIndex = 10;
			this->comboBox1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox1_SelectedIndexChanged);
			// 
			// comboBox2
			// 
			this->comboBox2->FormattingEnabled = true;
			this->comboBox2->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"IPC", L"UPC", L"Assignee", L"Country"});
			this->comboBox2->Location = System::Drawing::Point(399, 121);
			this->comboBox2->Name = L"comboBox2";
			this->comboBox2->Size = System::Drawing::Size(92, 20);
			this->comboBox2->TabIndex = 11;
			this->comboBox2->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox2_SelectedIndexChanged);
			// 
			// comboBox3
			// 
			this->comboBox3->FormattingEnabled = true;
			this->comboBox3->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"IPC", L"UPC", L"Assignee", L"Country"});
			this->comboBox3->Location = System::Drawing::Point(399, 161);
			this->comboBox3->Name = L"comboBox3";
			this->comboBox3->Size = System::Drawing::Size(92, 20);
			this->comboBox3->TabIndex = 12;
			this->comboBox3->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox3_SelectedIndexChanged);
			// 
			// comboBox4
			// 
			this->comboBox4->FormattingEnabled = true;
			this->comboBox4->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"IPC", L"UPC", L"Assignee", L"Country"});
			this->comboBox4->Location = System::Drawing::Point(399, 201);
			this->comboBox4->Name = L"comboBox4";
			this->comboBox4->Size = System::Drawing::Size(92, 20);
			this->comboBox4->TabIndex = 13;
			this->comboBox4->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox4_SelectedIndexChanged);
			// 
			// comboBox5
			// 
			this->comboBox5->FormattingEnabled = true;
			this->comboBox5->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"IPC", L"UPC", L"Assignee", L"Country"});
			this->comboBox5->Location = System::Drawing::Point(399, 238);
			this->comboBox5->Name = L"comboBox5";
			this->comboBox5->Size = System::Drawing::Size(92, 20);
			this->comboBox5->TabIndex = 14;
			this->comboBox5->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox5_SelectedIndexChanged);
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(29, 62);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(55, 12);
			this->label1->TabIndex = 15;
			this->label1->Text = L"Clan Code";
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(294, 62);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(60, 12);
			this->label2->TabIndex = 16;
			this->label2->Text = L"Boost Ratio";
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(397, 62);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(60, 12);
			this->label3->TabIndex = 17;
			this->label3->Text = L"Patent Type";
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(31, 280);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(118, 23);
			this->button1->TabIndex = 18;
			this->button1->Text = L"Exit and Save";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form2::button1_Click);
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(496, 280);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(125, 23);
			this->button3->TabIndex = 20;
			this->button3->Text = L"Exit and Discard";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &Form2::button3_Click);
			// 
			// textBox11
			// 
			this->textBox11->Location = System::Drawing::Point(31, 22);
			this->textBox11->Name = L"textBox11";
			this->textBox11->Size = System::Drawing::Size(469, 22);
			this->textBox11->TabIndex = 23;
			this->textBox11->TextChanged += gcnew System::EventHandler(this, &Form2::textBox11_TextChanged);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(245, 280);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(153, 23);
			this->button2->TabIndex = 24;
			this->button2->Text = L"Exit and Save to a New File";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &Form2::button2_Click);
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->FileName = L"openFileDialog1";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(523, 62);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(58, 12);
			this->label4->TabIndex = 25;
			this->label4->Text = L"Paper Type";
			// 
			// comboBox6
			// 
			this->comboBox6->FormattingEnabled = true;
			this->comboBox6->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"WOS", L"Author"});
			this->comboBox6->Location = System::Drawing::Point(525, 83);
			this->comboBox6->Name = L"comboBox6";
			this->comboBox6->Size = System::Drawing::Size(87, 20);
			this->comboBox6->TabIndex = 26;
			this->comboBox6->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox6_SelectedIndexChanged);
			// 
			// comboBox7
			// 
			this->comboBox7->FormattingEnabled = true;
			this->comboBox7->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"WOS", L"Author"});
			this->comboBox7->Location = System::Drawing::Point(525, 121);
			this->comboBox7->Name = L"comboBox7";
			this->comboBox7->Size = System::Drawing::Size(87, 20);
			this->comboBox7->TabIndex = 27;
			this->comboBox7->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox7_SelectedIndexChanged);
			// 
			// comboBox8
			// 
			this->comboBox8->FormattingEnabled = true;
			this->comboBox8->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"WOS", L"Author"});
			this->comboBox8->Location = System::Drawing::Point(525, 161);
			this->comboBox8->Name = L"comboBox8";
			this->comboBox8->Size = System::Drawing::Size(87, 20);
			this->comboBox8->TabIndex = 28;
			this->comboBox8->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox8_SelectedIndexChanged);
			// 
			// comboBox9
			// 
			this->comboBox9->FormattingEnabled = true;
			this->comboBox9->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"WOS", L"Author"});
			this->comboBox9->Location = System::Drawing::Point(525, 201);
			this->comboBox9->Name = L"comboBox9";
			this->comboBox9->Size = System::Drawing::Size(87, 20);
			this->comboBox9->TabIndex = 29;
			this->comboBox9->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox9_SelectedIndexChanged);
			// 
			// comboBox10
			// 
			this->comboBox10->FormattingEnabled = true;
			this->comboBox10->Items->AddRange(gcnew cli::array< System::Object^  >(2) {L"WOS", L"Author"});
			this->comboBox10->Location = System::Drawing::Point(525, 240);
			this->comboBox10->Name = L"comboBox10";
			this->comboBox10->Size = System::Drawing::Size(87, 20);
			this->comboBox10->TabIndex = 30;
			this->comboBox10->SelectedIndexChanged += gcnew System::EventHandler(this, &Form2::comboBox10_SelectedIndexChanged);
			// 
			// Form2
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 12);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(642, 320);
			this->Controls->Add(this->comboBox10);
			this->Controls->Add(this->comboBox9);
			this->Controls->Add(this->comboBox8);
			this->Controls->Add(this->comboBox7);
			this->Controls->Add(this->comboBox6);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->textBox11);
			this->Controls->Add(this->button3);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->comboBox5);
			this->Controls->Add(this->comboBox4);
			this->Controls->Add(this->comboBox3);
			this->Controls->Add(this->comboBox2);
			this->Controls->Add(this->comboBox1);
			this->Controls->Add(this->textBox10);
			this->Controls->Add(this->textBox9);
			this->Controls->Add(this->textBox8);
			this->Controls->Add(this->textBox7);
			this->Controls->Add(this->textBox6);
			this->Controls->Add(this->textBox5);
			this->Controls->Add(this->textBox4);
			this->Controls->Add(this->textBox3);
			this->Controls->Add(this->textBox2);
			this->Controls->Add(this->textBox1);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->Name = L"Form2";
			this->Text = L"Clan Analysis Parameters";
			this->Load += gcnew System::EventHandler(this, &Form2::Form2_Load);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void comboBox1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ctype1 = comboBox1->SelectedIndex+1;
			 }
	private: System::Void comboBox2_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ctype2 = comboBox2->SelectedIndex+1;
			 }
	private: System::Void comboBox3_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ctype3 = comboBox3->SelectedIndex+1;
			 }
	private: System::Void comboBox4_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ctype4 = comboBox4->SelectedIndex+1;
			 }
	private: System::Void comboBox5_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ctype5 = comboBox5->SelectedIndex+1;
			 }
	private: System::Void Form2_Load(System::Object^  sender, System::EventArgs^  e) {
			 FILE *cstream; wchar_t fname[FNAME_SIZE]; int i;
			 for (i = 0; i < clanspec->Length; i++)
				 fname[i] = clanspec[i]; 
			 fname[i] = '\0';
			 ccode1 = ""; ccode2 = ""; ccode3 = ""; ccode4 = ""; ccode5 = "";
			 bratio1 = L"1.0"; bratio2 = L"1.0"; bratio3 = L"1.0"; bratio4 = L"1.0"; bratio5 = L"1.0";
			 textBox11->Enabled = false;
			 if (ftype ==  WOS_DATA || ftype == WOS_MULTIPLE_DATA)
			 {
				comboBox1->Enabled = false;	comboBox2->Enabled = false; comboBox3->Enabled = false; comboBox4->Enabled = false; comboBox5->Enabled = false;
				comboBox6->Enabled = true; comboBox7->Enabled = true; comboBox8->Enabled = true; comboBox9->Enabled = true; comboBox10->Enabled = true;
			 } 
			 else if (ftype == USPTO_DATA || ftype == THOMSON_INNOVATION_DATA || ftype == WEBPAT2_DATA || ftype == WEBPAT3_DATA || ftype == WEBPAT3_MULTIPLE_DATA || ftype == PGUIDER_DATA)
			 {
				comboBox1->Enabled = true;	comboBox2->Enabled = true; comboBox3->Enabled = true; comboBox4->Enabled = true; comboBox5->Enabled = true;
				comboBox6->Enabled = false; comboBox7->Enabled = false; comboBox8->Enabled = false; comboBox9->Enabled = false; comboBox10->Enabled = false;
			 } 
			 ctype1 = ctype2 = ctype3 = ctype4 = ctype5 = CTYPE_ASSIGNEE; 
			 ptype1 = ptype2 = ptype3 = ptype4 = ptype5 = PTYPE_WOS; 
			 if (_wfopen_s(&cstream, fname, L"rt, ccs=UTF-8") != 0)	// check if the clan specification file exist, changed to support unicode, 2017/03/02
			 {
				 if (fname[0] == '\0' || fname[0] == ' ')	// file name not given
				 {
					 textBox11->Text = clanspec;	 
					 button1->Enabled = false;
				 }
				 else
				 {
					 textBox11->Text = clanspec;	 	 
					 button1->Enabled = true;
				 }
				 textBox6->Text = L"1.0";
				 textBox7->Text = L"1.0";
				 textBox8->Text = L"1.0";
				 textBox9->Text = L"1.0";
				 textBox10->Text = L"1.0";
				 ctype1 = CTYPE_ASSIGNEE; comboBox1->SelectedIndex=ctype1 - 1; 
				 ctype2 = CTYPE_ASSIGNEE; comboBox2->SelectedIndex=ctype2 - 1; 
				 ctype3 = CTYPE_ASSIGNEE; comboBox3->SelectedIndex=ctype3 - 1; 
				 ctype4 = CTYPE_ASSIGNEE; comboBox4->SelectedIndex=ctype4 - 1; 
				 ctype5 = CTYPE_ASSIGNEE; comboBox5->SelectedIndex=ctype5 - 1; 
				 ptype1 = PTYPE_WOS; comboBox6->SelectedIndex=ptype1 - 1; 
				 ptype2 = PTYPE_WOS; comboBox7->SelectedIndex=ptype2 - 1; 
				 ptype3 = PTYPE_WOS; comboBox8->SelectedIndex=ptype3 - 1; 
				 ptype4 = PTYPE_WOS; comboBox9->SelectedIndex=ptype4 - 1; 
				 ptype5 = PTYPE_WOS; comboBox10->SelectedIndex=ptype5 - 1; 
			 }
			 else
			 {		 
				 #define LBUF_SIZE 1024		
				 wchar_t line[LBUF_SIZE], buf1[LBUF_SIZE], buf2[LBUF_SIZE], buf3[LBUF_SIZE];
				 int ret;
				 ctype1 = ctype2 = ctype3 = ctype4 = ctype5 = 0; 
				 ptype1 = ptype2 = ptype3 = ptype4 = ptype5 = 0; 
				 if(fgetws(line, LBUF_SIZE, cstream) != NULL) // read the 1st line
				 {
					 if (ret = wcscmp(line, L"*** CLAN SPECIFICATION ***\n") != 0)
					 {
						System::Windows::Forms::MessageBox::Show("Invalid Clan Specification File!");
						this->Close();
					 }
				 }
				 textBox11->Text = clanspec;
				 if(fgetws(line, LBUF_SIZE, cstream) != NULL) 
				 {
					 parse_clan_line(line, buf1, buf2, buf3);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA)
					 {
						if (wcscmp(buf1, L"WOS") == 0) ptype1 = PTYPE_WOS; else if (wcscmp(buf1, L"Author") == 0) ptype1 = PTYPE_AUTHOR; else ptype1 = 0;
					 }
					 else
					 {
						if (wcscmp(buf1, L"IPC") == 0) ctype1 = CTYPE_IPC; else if (wcscmp(buf1, L"UPC") == 0) ctype1 = CTYPE_UPC; else if (wcscmp(buf1, L"Assignee") == 0) ctype1 = CTYPE_ASSIGNEE; else if (wcscmp(buf1, L"Country") == 0) ctype1 = CTYPE_COUNTRY; else ctype1 = 0;
					 }
					 if (ctype1 != 0 || ptype1 != 0)
					 {
						 bratio1 = L"";
						 for (i = 0; i < (int)wcslen(buf2); i++) bratio1 = String::Concat(bratio1, Convert::ToString(buf2[i]));
						 for (i = 0; i < (int)wcslen(buf3); i++) ccode1 = String::Concat(ccode1, Convert::ToString(buf3[i]));
					 }
				 }				 
				 if(fgetws(line, LBUF_SIZE, cstream) != NULL) 
				 {
					 parse_clan_line(line, buf1, buf2, buf3);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA)
					 {
						if (wcscmp(buf1, L"WOS") == 0) ptype2 = PTYPE_WOS; else if (wcscmp(buf1, L"Author") == 0) ptype2 = PTYPE_AUTHOR; else ptype2 = 0;
					 }
					 else
					 {
						if (wcscmp(buf1, L"IPC") == 0) ctype2 = CTYPE_IPC; else if (wcscmp(buf1, L"UPC") == 0) ctype2 = CTYPE_UPC; else if (wcscmp(buf1, L"Assignee") == 0) ctype2 = CTYPE_ASSIGNEE; else if (wcscmp(buf1, L"Country") == 0) ctype2 = CTYPE_COUNTRY; else ctype2 = 0;
					 }					 
					 if (ctype2 != 0 || ptype2 != 0)
					 {
						 bratio2 = L"";
						 for (i = 0; i < (int)wcslen(buf2); i++) bratio2 = String::Concat(bratio2, Convert::ToString(buf2[i]));
						 for (i = 0; i < (int)wcslen(buf3); i++) ccode2 = String::Concat(ccode2, Convert::ToString(buf3[i]));
					 }
				 }				 
				 if(fgetws(line, LBUF_SIZE, cstream) != NULL) 
				 {
					 parse_clan_line(line, buf1, buf2, buf3);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA)
					 {
						if (wcscmp(buf1, L"WOS") == 0) ptype3 = PTYPE_WOS; else if (wcscmp(buf1, L"Author") == 0) ptype3 = PTYPE_AUTHOR; else ptype3 = 0;
					 }
					 else
					 {
						if (wcscmp(buf1, L"IPC") == 0) ctype3 = CTYPE_IPC; else if (wcscmp(buf1, L"UPC") == 0) ctype3 = CTYPE_UPC; else if (wcscmp(buf1, L"Assignee") == 0) ctype3 = CTYPE_ASSIGNEE; else if (wcscmp(buf1, L"Country") == 0) ctype3 = CTYPE_COUNTRY; else ctype3 = 0;
					 }					 
					 if (ctype3 != 0 || ptype3 != 0)
					 {
						 bratio3 = L"";
						 for (i = 0; i < (int)wcslen(buf2); i++) bratio3 = String::Concat(bratio3, Convert::ToString(buf2[i]));
						 for (i = 0; i < (int)wcslen(buf3); i++) ccode3 = String::Concat(ccode3, Convert::ToString(buf3[i]));
					 }
				 }				 
				 if(fgetws(line, LBUF_SIZE, cstream) != NULL) 
				 {
					 parse_clan_line(line, buf1, buf2, buf3);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA)
					 {
						if (wcscmp(buf1, L"WOS") == 0) ptype4 = PTYPE_WOS; else if (wcscmp(buf1, L"Author") == 0) ptype4 = PTYPE_AUTHOR; else ptype4 = 0;
					 }
					 else
					 {
						if (wcscmp(buf1, L"IPC") == 0) ctype4 = CTYPE_IPC; else if (wcscmp(buf1, L"UPC") == 0) ctype4 = CTYPE_UPC; else if (wcscmp(buf1, L"Assignee") == 0) ctype4 = CTYPE_ASSIGNEE; else if (wcscmp(buf1, L"Country") == 0) ctype4 = CTYPE_COUNTRY; else ctype4 = 0;
					 }
					 if (ctype4 != 0 || ptype4 != 0)
					 {
						 bratio4 = L"";
						 for (i = 0; i < (int)wcslen(buf2); i++) bratio4 = String::Concat(bratio4, Convert::ToString(buf2[i]));
						 for (i = 0; i < (int)wcslen(buf3); i++) ccode4 = String::Concat(ccode4, Convert::ToString(buf3[i]));
					 }
				 }				 
				 if(fgetws(line, LBUF_SIZE, cstream) != NULL) 
				 {
					 parse_clan_line(line, buf1, buf2, buf3);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA)
					 {
						if (wcscmp(buf1, L"WOS") == 0) ptype5 = PTYPE_WOS; else if (wcscmp(buf1, L"Author") == 0) ptype5 = PTYPE_AUTHOR; else ptype5 = 0;
					 }
					 else
					 {
						if (wcscmp(buf1, L"IPC") == 0) ctype5 = CTYPE_IPC; else if (wcscmp(buf1, L"UPC") == 0) ctype5 = CTYPE_UPC; else if (wcscmp(buf1, L"Assignee") == 0) ctype5 = CTYPE_ASSIGNEE; else if (wcscmp(buf1, L"Country") == 0) ctype5 = CTYPE_COUNTRY; else ctype5 = 0;
					 }
					 if (ctype5 != 0 || ptype5 != 0)
					 {
						 bratio5 = L"";
						 for (i = 0; i < (int)wcslen(buf2); i++) bratio5 = String::Concat(bratio5, Convert::ToString(buf2[i]));
						 for (i = 0; i < (int)wcslen(buf3); i++) ccode5 = String::Concat(ccode5, Convert::ToString(buf3[i]));
					 }
				 }
				 fclose(cstream);
				 textBox1->Text = ccode1; textBox6->Text = bratio1;  
				 textBox2->Text = ccode2; textBox7->Text = bratio2;  
				 textBox3->Text = ccode3; textBox8->Text = bratio3;
				 textBox4->Text = ccode4; textBox9->Text = bratio4; 
				 textBox5->Text = ccode5; textBox10->Text = bratio5; 
				 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA)
				 {
					comboBox6->SelectedIndex=ptype1 - 1;
					comboBox7->SelectedIndex=ptype2 - 1;
					comboBox8->SelectedIndex=ptype3 - 1; 
					comboBox9->SelectedIndex=ptype4 - 1;
					comboBox10->SelectedIndex=ptype5 - 1; 
				 }
				 else
				 {
					comboBox1->SelectedIndex=ctype1 - 1;
					comboBox2->SelectedIndex=ctype2 - 1;
					comboBox3->SelectedIndex=ctype3 - 1; 
					comboBox4->SelectedIndex=ctype4 - 1;
					comboBox5->SelectedIndex=ctype5 - 1; 
				 }
			 }
			 }
	private: System::Void textBox1_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 ccode1 = textBox1->Text;
			 }
	private: System::Void textBox2_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 ccode2 = textBox2->Text;
			 }
	private: System::Void textBox3_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 ccode3 = textBox3->Text;
			 }
	private: System::Void textBox4_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 ccode4 = textBox4->Text;
			 }
	private: System::Void textBox5_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 ccode5 = textBox5->Text;
			 }
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
				 FILE *cstream; wchar_t fname[FNAME_SIZE]; int i; wchar_t ty[100]; wchar_t br[100]; wchar_t cc[100];
				 String ^typestr;
				 for (i = 0; i < clanspec->Length; i++)
					 fname[i] = clanspec[i]; 
				 fname[i] = '\0';
				 if (_wfopen_s(&cstream, fname, L"wt, ccs=UTF-8") == 0)	// open for writing, changed to support unicode, 2017/03/02
				 {
					 fwprintf(cstream, L"*** CLAN SPECIFICATION ***\n");
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
						if (ptype1 == PTYPE_WOS) typestr="WOS"; else if (ptype1 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
					 else {
						if (ctype1 == CTYPE_IPC) typestr="IPC"; else if (ctype1 == CTYPE_UPC) typestr="UPC"; else if (ctype1 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype1 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
					 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
					 for (i = 0; i < bratio1->Length; i++) br[i] = bratio1[i]; br[i] = '\0';
					 for (i = 0; i < ccode1->Length; i++) cc[i] = ccode1[i]; cc[i] = '\0';
					 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
						if (ptype2 == PTYPE_WOS) typestr="WOS"; else if (ptype2 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
					 else {
						if (ctype2 == CTYPE_IPC) typestr="IPC"; else if (ctype2 == CTYPE_UPC) typestr="UPC"; else if (ctype2 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype2 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
					 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
					 for (i = 0; i < bratio2->Length; i++) br[i] = bratio2[i]; br[i] = '\0';
					 for (i = 0; i < ccode2->Length; i++) cc[i] = ccode2[i]; cc[i] = '\0';
					 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
						if (ptype3 == PTYPE_WOS) typestr="WOS"; else if (ptype3 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
					 else {
						if (ctype3 == CTYPE_IPC) typestr="IPC"; else if (ctype3 == CTYPE_UPC) typestr="UPC"; else if (ctype3 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype3 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
					 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
					 for (i = 0; i < bratio3->Length; i++) br[i] = bratio3[i]; br[i] = '\0';
					 for (i = 0; i < ccode3->Length; i++) cc[i] = ccode3[i]; cc[i] = '\0';
					 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
						if (ptype4 == PTYPE_WOS) typestr="WOS"; else if (ptype4 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
					 else {
						if (ctype4 == CTYPE_IPC) typestr="IPC"; else if (ctype4 == CTYPE_UPC) typestr="UPC"; else if (ctype4 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype4 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
					 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
					 for (i = 0; i < bratio4->Length; i++) br[i] = bratio4[i]; br[i] = '\0';
					 for (i = 0; i < ccode4->Length; i++) cc[i] = ccode4[i]; cc[i] = '\0';
					 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
					 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
						if (ptype5 == PTYPE_WOS) typestr="WOS"; else if (ptype5 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
					 else {
						if (ctype5 == CTYPE_IPC) typestr="IPC"; else if (ctype5 == CTYPE_UPC) typestr="UPC"; else if (ctype5 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype5 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
					 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
					 for (i = 0; i < bratio5->Length; i++) br[i] = bratio5[i]; br[i] = '\0';
					 for (i = 0; i < ccode5->Length; i++) cc[i] = ccode5[i]; cc[i] = '\0';
					 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
					 fclose(cstream);
					 exit_type = 1;	// exit and save to file
					 this->Close();
				 }
			 }
	private: System::Void button3_Click(System::Object^  sender, System::EventArgs^  e) {
				 exit_type = 2;	// exit and discard
				 this->Close();
			 }
	private: System::Void textBox6_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 bratio1 = textBox6->Text;
			 }
	private: System::Void textBox7_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 bratio2 = textBox7->Text;
			 }
	private: System::Void textBox8_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 bratio3 = textBox8->Text;
			 }
	private: System::Void textBox9_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 bratio4 = textBox9->Text;
			 }
	private: System::Void textBox10_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 bratio5 = textBox10->Text;
			 }
	private: System::Void textBox11_TextChanged(System::Object^  sender, System::EventArgs^  e) {
				 clanspec = textBox11->Text;
			 }
	private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {		
				 openFileDialog1->CheckFileExists = false;	// set to accept file that does not exist				 
				 if(openFileDialog1->ShowDialog()==System::Windows::Forms::DialogResult::OK) 
				 {
					 clanspec = openFileDialog1->FileName;
					 FILE *cstream; wchar_t fname[FNAME_SIZE]; int i; wchar_t ty[100]; wchar_t br[100]; wchar_t cc[100];
					 String ^typestr;
					 for (i = 0; i < clanspec->Length; i++)
						 fname[i] = clanspec[i]; 
					 fname[i] = '\0';

					 if (_wfopen_s(&cstream, fname, L"wt, ccs=UTF-8") == 0)	// open for writing
					 {
						 fwprintf(cstream, L"*** CLAN SPECIFICATION ***\n");
						 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
							if (ptype1 == PTYPE_WOS) typestr="WOS"; else if (ptype1 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
						 else {
							if (ctype1 == CTYPE_IPC) typestr="IPC"; else if (ctype1 == CTYPE_UPC) typestr="UPC"; else if (ctype1 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype1 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
						 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
						 for (i = 0; i < bratio1->Length; i++) br[i] = bratio1[i]; br[i] = '\0';
						 for (i = 0; i < ccode1->Length; i++) cc[i] = ccode1[i]; cc[i] = '\0';
						 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);	
						 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
							if (ptype2 == PTYPE_WOS) typestr="WOS"; else if (ptype2 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
						 else {
							if (ctype2 == CTYPE_IPC) typestr="IPC"; else if (ctype2 == CTYPE_UPC) typestr="UPC"; else if (ctype2 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype2 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
						 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
						 for (i = 0; i < bratio2->Length; i++) br[i] = bratio2[i]; br[i] = '\0';
						 for (i = 0; i < ccode2->Length; i++) cc[i] = ccode2[i]; cc[i] = '\0';
						 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
						 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
							if (ptype3 == PTYPE_WOS) typestr="WOS"; else if (ptype3 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
						 else {
							if (ctype3 == CTYPE_IPC) typestr="IPC"; else if (ctype3 == CTYPE_UPC) typestr="UPC"; else if (ctype3 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype3 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
						 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
						 for (i = 0; i < bratio3->Length; i++) br[i] = bratio3[i]; br[i] = '\0';
						 for (i = 0; i < ccode3->Length; i++) cc[i] = ccode3[i]; cc[i] = '\0';
						 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
						 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
							if (ptype4 == PTYPE_WOS) typestr="WOS"; else if (ptype4 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
						 else {
							if (ctype4 == CTYPE_IPC) typestr="IPC"; else if (ctype4 == CTYPE_UPC) typestr="UPC"; else if (ctype4 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype4 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
						 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
						 for (i = 0; i < bratio4->Length; i++) br[i] = bratio4[i]; br[i] = '\0';
						 for (i = 0; i < ccode4->Length; i++) cc[i] = ccode4[i]; cc[i] = '\0';
						 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
						 if (ftype == WOS_DATA || ftype == WOS_MULTIPLE_DATA) {
							if (ptype5 == PTYPE_WOS) typestr="WOS"; else if (ptype5 == PTYPE_AUTHOR) typestr="Author"; else typestr="?????"; }
						 else {
							if (ctype5 == CTYPE_IPC) typestr="IPC"; else if (ctype5 == CTYPE_UPC) typestr="UPC"; else if (ctype5 == CTYPE_ASSIGNEE) typestr="Assignee"; else if (ctype5 == CTYPE_COUNTRY) typestr="Country"; else typestr="?????"; }
						 for (i = 0; i < typestr->Length; i++) ty[i] = typestr[i]; ty[i] = '\0';
						 for (i = 0; i < bratio5->Length; i++) br[i] = bratio5[i]; br[i] = '\0';
						 for (i = 0; i < ccode5->Length; i++) cc[i] = ccode5[i]; cc[i] = '\0';
						 if (cc[0] != '\0' && cc[0] != ' ' && br[0] != '\0' && br[0] != ' ') fwprintf(cstream, L"%s\t%s\t%s\n", ty, br, cc);
						 fclose(cstream);
						 exit_type = 1;	// exit and save to file
						 this->Close();
					 }
				 }
		 }
	private: System::Void comboBox6_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ptype1 = comboBox6->SelectedIndex+1;
			 }
	private: System::Void comboBox7_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ptype2 = comboBox7->SelectedIndex+1;
		 }
	private: System::Void comboBox8_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ptype3 = comboBox8->SelectedIndex+1;
		 }
	private: System::Void comboBox9_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ptype4 = comboBox9->SelectedIndex+1;
		 }
	private: System::Void comboBox10_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 ptype5 = comboBox10->SelectedIndex+1;
		 }
};
}
