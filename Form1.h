#pragma once

#include "stdafx.h"
#include "resource.h"
#include <windows.h>
#include "Form2.h"
#include "FormLaw.h"

namespace MainPath {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Form1 的摘要
	///
	/// 警告: 如果您變更這個類別的名稱，就必須變更與這個類別所依據之所有 .resx 檔案關聯的
	///          Managed 資源編譯器工具的 'Resource File Name' 屬性。
	///          否則，這些設計工具
	///          將無法與這個表單關聯的當地語系化資源
	///          正確互動。
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
		int NetworkType;
		int Directionality;
		int Direction;
		int Separator;
		int SPMethod;
		int SCOnly;
		int MPType;
		int BPType;
		int release_id;
		int version_id;
		int FType;	// WOS or patent data, etc.
		int GroupFinderOptions;	// added 2014/04/11, label modified to "Clustering Citaton Network", 2016/07/07, changed to "Clustering Network"
		int ClusteringCoauthorNetworkOptions;	// added 2015/10/19, after 2016/07/05, the variable is made to be determined by GroupFinderOptions
		int RelevancyStrategy;	// added 2016/05/07
		int UnlockLicense;		// added 2016/10/08
		bool legal;
	private: System::Windows::Forms::ComboBox^  comboBox1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::ComboBox^  comboBox2;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::ComboBox^  comboBox3;





	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::TextBox^  textBox3;
	private: System::Windows::Forms::RadioButton^  radioButton1;
	private: System::Windows::Forms::RadioButton^  radioButton2;
	private: System::Windows::Forms::GroupBox^  groupBox1;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::TextBox^  textBox4;
	private: System::Windows::Forms::Button^  button5;


	private: System::Windows::Forms::RadioButton^  radioButton5;
	private: System::Windows::Forms::RadioButton^  radioButton6;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::Label^  label9;
	private: System::Windows::Forms::TextBox^  textBox5;
	private: System::Windows::Forms::TextBox^  textBox6;
	private: System::Windows::Forms::GroupBox^  groupBox2;

	private: System::Windows::Forms::RadioButton^  radioButton7;
	private: System::Windows::Forms::TextBox^  textBox7;
	private: System::Windows::Forms::Label^  label10;
	private: System::Windows::Forms::RadioButton^  radioButton8;
	private: System::Windows::Forms::TextBox^  textBox2;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::Button^  button6;
	private: System::Windows::Forms::Label^  label11;
	private: System::Windows::Forms::TextBox^  textBox8;
	private: System::Windows::Forms::Label^  label12;
	private: System::Windows::Forms::TextBox^  textBox9;
	private: System::Windows::Forms::Label^  label13;
	private: System::Windows::Forms::TextBox^  textBox10;
	private: System::Windows::Forms::RadioButton^  radioButton3;
	private: System::Windows::Forms::Label^  label14;
	private: System::Windows::Forms::Label^  label15;
	private: System::Windows::Forms::RadioButton^  radioButton4;
	private: System::Windows::Forms::Label^  label16;
	private: System::Windows::Forms::TextBox^  textBox11;
	private: System::Windows::Forms::Label^  label17;
	private: System::Windows::Forms::Label^  label18;
	private: System::Windows::Forms::Label^  label19;
	private: System::Windows::Forms::Label^  label20;
	public: System::Windows::Forms::TextBox^  textBox12;	// changed from "private", 2011/06/19


	private: System::Windows::Forms::GroupBox^  groupBox3;



	private: System::Windows::Forms::RadioButton^  radioButton12;
	private: System::Windows::Forms::Button^  button7;
	private: System::Windows::Forms::Button^  button8;
	private: System::Windows::Forms::Label^  label21;
	private: System::Windows::Forms::TextBox^  textBox13;
	private: System::Windows::Forms::Label^  label22;
	private: System::Windows::Forms::ComboBox^  comboBox4;


	private: System::Windows::Forms::Label^  label24;
	private: System::Windows::Forms::ComboBox^  comboBox6;
	private: System::Windows::Forms::Label^  label3;
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: 在此加入建構函式程式碼
			//
		}
	protected:
		/// <summary>
		/// 清除任何使用中的資源。
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  label1;
	protected: 
	private: System::Windows::Forms::TextBox^  textBox1;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Button^  button2;
	private: System::Windows::Forms::Button^  button3;
	private: System::Windows::Forms::Button^  button4;
	private: System::Windows::Forms::OpenFileDialog^  openFileDialog1;

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
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(Form1::typeid));
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->button3 = (gcnew System::Windows::Forms::Button());
			this->button4 = (gcnew System::Windows::Forms::Button());
			this->openFileDialog1 = (gcnew System::Windows::Forms::OpenFileDialog());
			this->comboBox1 = (gcnew System::Windows::Forms::ComboBox());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->comboBox2 = (gcnew System::Windows::Forms::ComboBox());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->comboBox3 = (gcnew System::Windows::Forms::ComboBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->textBox3 = (gcnew System::Windows::Forms::TextBox());
			this->radioButton1 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton2 = (gcnew System::Windows::Forms::RadioButton());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->textBox11 = (gcnew System::Windows::Forms::TextBox());
			this->label16 = (gcnew System::Windows::Forms::Label());
			this->radioButton4 = (gcnew System::Windows::Forms::RadioButton());
			this->label15 = (gcnew System::Windows::Forms::Label());
			this->label14 = (gcnew System::Windows::Forms::Label());
			this->label13 = (gcnew System::Windows::Forms::Label());
			this->textBox10 = (gcnew System::Windows::Forms::TextBox());
			this->radioButton3 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton8 = (gcnew System::Windows::Forms::RadioButton());
			this->textBox7 = (gcnew System::Windows::Forms::TextBox());
			this->label10 = (gcnew System::Windows::Forms::Label());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->textBox4 = (gcnew System::Windows::Forms::TextBox());
			this->button5 = (gcnew System::Windows::Forms::Button());
			this->radioButton5 = (gcnew System::Windows::Forms::RadioButton());
			this->radioButton6 = (gcnew System::Windows::Forms::RadioButton());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->label9 = (gcnew System::Windows::Forms::Label());
			this->textBox5 = (gcnew System::Windows::Forms::TextBox());
			this->textBox6 = (gcnew System::Windows::Forms::TextBox());
			this->groupBox2 = (gcnew System::Windows::Forms::GroupBox());
			this->radioButton12 = (gcnew System::Windows::Forms::RadioButton());
			this->label19 = (gcnew System::Windows::Forms::Label());
			this->label18 = (gcnew System::Windows::Forms::Label());
			this->radioButton7 = (gcnew System::Windows::Forms::RadioButton());
			this->textBox2 = (gcnew System::Windows::Forms::TextBox());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->button6 = (gcnew System::Windows::Forms::Button());
			this->label11 = (gcnew System::Windows::Forms::Label());
			this->textBox8 = (gcnew System::Windows::Forms::TextBox());
			this->label12 = (gcnew System::Windows::Forms::Label());
			this->textBox9 = (gcnew System::Windows::Forms::TextBox());
			this->label17 = (gcnew System::Windows::Forms::Label());
			this->label20 = (gcnew System::Windows::Forms::Label());
			this->textBox12 = (gcnew System::Windows::Forms::TextBox());
			this->groupBox3 = (gcnew System::Windows::Forms::GroupBox());
			this->button8 = (gcnew System::Windows::Forms::Button());
			this->button7 = (gcnew System::Windows::Forms::Button());
			this->label21 = (gcnew System::Windows::Forms::Label());
			this->textBox13 = (gcnew System::Windows::Forms::TextBox());
			this->label22 = (gcnew System::Windows::Forms::Label());
			this->comboBox4 = (gcnew System::Windows::Forms::ComboBox());
			this->label24 = (gcnew System::Windows::Forms::Label());
			this->comboBox6 = (gcnew System::Windows::Forms::ComboBox());
			this->groupBox1->SuspendLayout();
			this->groupBox2->SuspendLayout();
			this->groupBox3->SuspendLayout();
			this->SuspendLayout();
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(56, 100);
			this->label1->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(103, 15);
			this->label1->TabIndex = 0;
			this->label1->Text = L"Relationship List";
			// 
			// textBox1
			// 
			this->textBox1->Location = System::Drawing::Point(175, 39);
			this->textBox1->Margin = System::Windows::Forms::Padding(4);
			this->textBox1->Name = L"textBox1";
			this->textBox1->Size = System::Drawing::Size(557, 25);
			this->textBox1->TabIndex = 1;
			this->textBox1->TextChanged += gcnew System::EventHandler(this, &Form1::textBox1_TextChanged);
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(767, 91);
			this->button1->Margin = System::Windows::Forms::Padding(4);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(100, 29);
			this->button1->TabIndex = 2;
			this->button1->Text = L"Browse";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &Form1::button1_Click);
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(908, 38);
			this->button2->Margin = System::Windows::Forms::Padding(4);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(100, 29);
			this->button2->TabIndex = 3;
			this->button2->Text = L"Info";
			this->button2->UseVisualStyleBackColor = true;
			this->button2->Click += gcnew System::EventHandler(this, &Form1::button2_Click);
			// 
			// button3
			// 
			this->button3->Location = System::Drawing::Point(908, 81);
			this->button3->Margin = System::Windows::Forms::Padding(4);
			this->button3->Name = L"button3";
			this->button3->Size = System::Drawing::Size(100, 49);
			this->button3->TabIndex = 4;
			this->button3->Text = L"Execute";
			this->button3->UseVisualStyleBackColor = true;
			this->button3->Click += gcnew System::EventHandler(this, &Form1::button3_Click);
			// 
			// button4
			// 
			this->button4->Location = System::Drawing::Point(908, 149);
			this->button4->Margin = System::Windows::Forms::Padding(4);
			this->button4->Name = L"button4";
			this->button4->Size = System::Drawing::Size(100, 29);
			this->button4->TabIndex = 5;
			this->button4->Text = L"Exit";
			this->button4->UseVisualStyleBackColor = true;
			this->button4->Click += gcnew System::EventHandler(this, &Form1::button4_Click);
			// 
			// openFileDialog1
			// 
			this->openFileDialog1->FileName = L"openFileDialog1";
			// 
			// comboBox1
			// 
			this->comboBox1->FormattingEnabled = true;
			this->comboBox1->Items->AddRange(gcnew cli::array< System::Object^  >(7) {L"Tab", L"Space", L"Tab or Space", L";", L",", 
				L"/", L"\\"});
			this->comboBox1->Location = System::Drawing::Point(873, 287);
			this->comboBox1->Margin = System::Windows::Forms::Padding(4);
			this->comboBox1->Name = L"comboBox1";
			this->comboBox1->Size = System::Drawing::Size(160, 23);
			this->comboBox1->TabIndex = 6;
			this->comboBox1->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::comboBox1_SelectedIndexChanged);
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(871, 268);
			this->label2->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(61, 15);
			this->label2->TabIndex = 7;
			this->label2->Text = L"Separator";
			// 
			// comboBox2
			// 
			this->comboBox2->FormattingEnabled = true;
			this->comboBox2->Items->AddRange(gcnew cli::array< System::Object^  >(6) {L"SPC", L"SPLC", L"SPNP", L"SPAD", L"SPGD", L"SPHD"});
			this->comboBox2->Location = System::Drawing::Point(677, 594);
			this->comboBox2->Margin = System::Windows::Forms::Padding(4);
			this->comboBox2->Name = L"comboBox2";
			this->comboBox2->Size = System::Drawing::Size(169, 23);
			this->comboBox2->TabIndex = 8;
			this->comboBox2->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::comboBox2_SelectedIndexChanged);
			// 
			// label3
			// 
			this->label3->AutoSize = true;
			this->label3->Location = System::Drawing::Point(674, 575);
			this->label3->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label3->Name = L"label3";
			this->label3->Size = System::Drawing::Size(160, 15);
			this->label3->TabIndex = 9;
			this->label3->Text = L"Search Path Count Method";
			// 
			// label4
			// 
			this->label4->AutoSize = true;
			this->label4->Location = System::Drawing::Point(674, 450);
			this->label4->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label4->Name = L"label4";
			this->label4->Size = System::Drawing::Size(75, 15);
			this->label4->TabIndex = 10;
			this->label4->Text = L"Node Scope";
			// 
			// comboBox3
			// 
			this->comboBox3->FormattingEnabled = true;
			this->comboBox3->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"Source Column Nodes Only", L"Nodes in Full Record Data", 
				L"All Nodes"});
			this->comboBox3->Location = System::Drawing::Point(677, 469);
			this->comboBox3->Margin = System::Windows::Forms::Padding(4);
			this->comboBox3->Name = L"comboBox3";
			this->comboBox3->Size = System::Drawing::Size(231, 23);
			this->comboBox3->TabIndex = 11;
			this->comboBox3->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::comboBox3_SelectedIndexChanged);
			// 
			// label6
			// 
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(248, 29);
			this->label6->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(260, 15);
			this->label6->TabIndex = 17;
			this->label6->Text = L"Tie Tolerance (fraction of the SPx tie value)";
			// 
			// textBox3
			// 
			this->textBox3->Location = System::Drawing::Point(529, 25);
			this->textBox3->Margin = System::Windows::Forms::Padding(4);
			this->textBox3->Name = L"textBox3";
			this->textBox3->Size = System::Drawing::Size(69, 25);
			this->textBox3->TabIndex = 18;
			// 
			// radioButton1
			// 
			this->radioButton1->AutoSize = true;
			this->radioButton1->Location = System::Drawing::Point(48, 59);
			this->radioButton1->Margin = System::Windows::Forms::Padding(4);
			this->radioButton1->Name = L"radioButton1";
			this->radioButton1->Size = System::Drawing::Size(76, 19);
			this->radioButton1->TabIndex = 14;
			this->radioButton1->TabStop = true;
			this->radioButton1->Text = L"Forward";
			this->radioButton1->UseVisualStyleBackColor = true;
			this->radioButton1->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton1_CheckedChanged);
			// 
			// radioButton2
			// 
			this->radioButton2->AutoSize = true;
			this->radioButton2->Location = System::Drawing::Point(49, 212);
			this->radioButton2->Margin = System::Windows::Forms::Padding(4);
			this->radioButton2->Name = L"radioButton2";
			this->radioButton2->Size = System::Drawing::Size(78, 19);
			this->radioButton2->TabIndex = 15;
			this->radioButton2->TabStop = true;
			this->radioButton2->Text = L"Standard";
			this->radioButton2->UseVisualStyleBackColor = true;
			this->radioButton2->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton2_CheckedChanged);
			// 
			// groupBox1
			// 
			this->groupBox1->Controls->Add(this->textBox11);
			this->groupBox1->Controls->Add(this->label16);
			this->groupBox1->Controls->Add(this->radioButton4);
			this->groupBox1->Controls->Add(this->label15);
			this->groupBox1->Controls->Add(this->label14);
			this->groupBox1->Controls->Add(this->label13);
			this->groupBox1->Controls->Add(this->textBox10);
			this->groupBox1->Controls->Add(this->radioButton3);
			this->groupBox1->Controls->Add(this->radioButton8);
			this->groupBox1->Controls->Add(this->textBox7);
			this->groupBox1->Controls->Add(this->label10);
			this->groupBox1->Controls->Add(this->radioButton2);
			this->groupBox1->Controls->Add(this->radioButton1);
			this->groupBox1->Controls->Add(this->label6);
			this->groupBox1->Controls->Add(this->textBox3);
			this->groupBox1->Location = System::Drawing::Point(33, 209);
			this->groupBox1->Margin = System::Windows::Forms::Padding(4);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Padding = System::Windows::Forms::Padding(4);
			this->groupBox1->Size = System::Drawing::Size(615, 285);
			this->groupBox1->TabIndex = 16;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"TYPE OF MAIN PATH SEARCH";
			// 
			// textBox11
			// 
			this->textBox11->Location = System::Drawing::Point(368, 238);
			this->textBox11->Margin = System::Windows::Forms::Padding(4);
			this->textBox11->Name = L"textBox11";
			this->textBox11->Size = System::Drawing::Size(71, 25);
			this->textBox11->TabIndex = 27;
			this->textBox11->TextChanged += gcnew System::EventHandler(this, &Form1::textBox11_TextChanged);
			// 
			// label16
			// 
			this->label16->AutoSize = true;
			this->label16->Location = System::Drawing::Point(161, 245);
			this->label16->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label16->Name = L"label16";
			this->label16->Size = System::Drawing::Size(177, 15);
			this->label16->TabIndex = 26;
			this->label16->Text = L"Number of Significant Routes";
			// 
			// radioButton4
			// 
			this->radioButton4->AutoSize = true;
			this->radioButton4->Location = System::Drawing::Point(49, 242);
			this->radioButton4->Margin = System::Windows::Forms::Padding(4);
			this->radioButton4->Name = L"radioButton4";
			this->radioButton4->Size = System::Drawing::Size(85, 19);
			this->radioButton4->TabIndex = 25;
			this->radioButton4->TabStop = true;
			this->radioButton4->Text = L"Key-route";
			this->radioButton4->UseVisualStyleBackColor = true;
			this->radioButton4->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton4_CheckedChanged);
			// 
			// label15
			// 
			this->label15->AutoSize = true;
			this->label15->Location = System::Drawing::Point(33, 179);
			this->label15->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label15->Name = L"label15";
			this->label15->Size = System::Drawing::Size(87, 15);
			this->label15->TabIndex = 24;
			this->label15->Text = L"Global Search";
			// 
			// label14
			// 
			this->label14->AutoSize = true;
			this->label14->Location = System::Drawing::Point(35, 29);
			this->label14->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label14->Name = L"label14";
			this->label14->Size = System::Drawing::Size(81, 15);
			this->label14->TabIndex = 23;
			this->label14->Text = L"Local Search";
			// 
			// label13
			// 
			this->label13->AutoSize = true;
			this->label13->Location = System::Drawing::Point(161, 134);
			this->label13->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label13->Name = L"label13";
			this->label13->Size = System::Drawing::Size(177, 15);
			this->label13->TabIndex = 22;
			this->label13->Text = L"Number of Significant Routes";
			// 
			// textBox10
			// 
			this->textBox10->Location = System::Drawing::Point(368, 124);
			this->textBox10->Margin = System::Windows::Forms::Padding(4);
			this->textBox10->Name = L"textBox10";
			this->textBox10->Size = System::Drawing::Size(71, 25);
			this->textBox10->TabIndex = 21;
			// 
			// radioButton3
			// 
			this->radioButton3->AutoSize = true;
			this->radioButton3->Location = System::Drawing::Point(48, 131);
			this->radioButton3->Margin = System::Windows::Forms::Padding(4);
			this->radioButton3->Name = L"radioButton3";
			this->radioButton3->Size = System::Drawing::Size(85, 19);
			this->radioButton3->TabIndex = 20;
			this->radioButton3->TabStop = true;
			this->radioButton3->Text = L"Key-route";
			this->radioButton3->UseVisualStyleBackColor = true;
			this->radioButton3->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton3_CheckedChanged);
			// 
			// radioButton8
			// 
			this->radioButton8->AutoSize = true;
			this->radioButton8->Location = System::Drawing::Point(48, 98);
			this->radioButton8->Margin = System::Windows::Forms::Padding(4);
			this->radioButton8->Name = L"radioButton8";
			this->radioButton8->Size = System::Drawing::Size(84, 19);
			this->radioButton8->TabIndex = 19;
			this->radioButton8->TabStop = true;
			this->radioButton8->Text = L"Backward";
			this->radioButton8->UseVisualStyleBackColor = true;
			this->radioButton8->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton8_CheckedChanged);
			// 
			// textBox7
			// 
			this->textBox7->Location = System::Drawing::Point(528, 172);
			this->textBox7->Margin = System::Windows::Forms::Padding(4);
			this->textBox7->Name = L"textBox7";
			this->textBox7->Size = System::Drawing::Size(69, 25);
			this->textBox7->TabIndex = 17;
			this->textBox7->TextChanged += gcnew System::EventHandler(this, &Form1::textBox7_TextChanged);
			// 
			// label10
			// 
			this->label10->AutoSize = true;
			this->label10->Location = System::Drawing::Point(303, 179);
			this->label10->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label10->Name = L"label10";
			this->label10->Size = System::Drawing::Size(202, 15);
			this->label10->TabIndex = 16;
			this->label10->Text = L"Number of Paths in Global Search";
			// 
			// label7
			// 
			this->label7->AutoSize = true;
			this->label7->Location = System::Drawing::Point(53, 48);
			this->label7->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label7->Name = L"label7";
			this->label7->Size = System::Drawing::Size(104, 15);
			this->label7->TabIndex = 19;
			this->label7->Text = L"Full Record Data";
			// 
			// textBox4
			// 
			this->textBox4->Location = System::Drawing::Point(175, 92);
			this->textBox4->Margin = System::Windows::Forms::Padding(4);
			this->textBox4->Name = L"textBox4";
			this->textBox4->Size = System::Drawing::Size(557, 25);
			this->textBox4->TabIndex = 20;
			this->textBox4->TextChanged += gcnew System::EventHandler(this, &Form1::textBox4_TextChanged);
			// 
			// button5
			// 
			this->button5->Location = System::Drawing::Point(767, 39);
			this->button5->Margin = System::Windows::Forms::Padding(4);
			this->button5->Name = L"button5";
			this->button5->Size = System::Drawing::Size(99, 28);
			this->button5->TabIndex = 21;
			this->button5->Text = L"Browse";
			this->button5->UseVisualStyleBackColor = true;
			this->button5->Click += gcnew System::EventHandler(this, &Form1::button5_Click);
			// 
			// radioButton5
			// 
			this->radioButton5->AutoSize = true;
			this->radioButton5->Location = System::Drawing::Point(211, 59);
			this->radioButton5->Margin = System::Windows::Forms::Padding(4);
			this->radioButton5->Name = L"radioButton5";
			this->radioButton5->Size = System::Drawing::Size(76, 19);
			this->radioButton5->TabIndex = 24;
			this->radioButton5->TabStop = true;
			this->radioButton5->Text = L"Forward";
			this->radioButton5->UseVisualStyleBackColor = true;
			this->radioButton5->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton5_CheckedChanged);
			// 
			// radioButton6
			// 
			this->radioButton6->AutoSize = true;
			this->radioButton6->Location = System::Drawing::Point(48, 128);
			this->radioButton6->Margin = System::Windows::Forms::Padding(4);
			this->radioButton6->Name = L"radioButton6";
			this->radioButton6->Size = System::Drawing::Size(78, 19);
			this->radioButton6->TabIndex = 25;
			this->radioButton6->TabStop = true;
			this->radioButton6->Text = L"Standard";
			this->radioButton6->UseVisualStyleBackColor = true;
			this->radioButton6->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton6_CheckedChanged);
			// 
			// label8
			// 
			this->label8->AutoSize = true;
			this->label8->Location = System::Drawing::Point(241, 26);
			this->label8->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label8->Name = L"label8";
			this->label8->Size = System::Drawing::Size(260, 15);
			this->label8->TabIndex = 26;
			this->label8->Text = L"Tie Tolerance (fraction of the SPx tie value)";
			// 
			// label9
			// 
			this->label9->AutoSize = true;
			this->label9->Location = System::Drawing::Point(300, 96);
			this->label9->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label9->Name = L"label9";
			this->label9->Size = System::Drawing::Size(202, 15);
			this->label9->TabIndex = 27;
			this->label9->Text = L"Number of Paths in Global Search";
			// 
			// textBox5
			// 
			this->textBox5->Location = System::Drawing::Point(529, 15);
			this->textBox5->Margin = System::Windows::Forms::Padding(4);
			this->textBox5->Name = L"textBox5";
			this->textBox5->Size = System::Drawing::Size(72, 25);
			this->textBox5->TabIndex = 28;
			// 
			// textBox6
			// 
			this->textBox6->Location = System::Drawing::Point(525, 89);
			this->textBox6->Margin = System::Windows::Forms::Padding(4);
			this->textBox6->Name = L"textBox6";
			this->textBox6->Size = System::Drawing::Size(73, 25);
			this->textBox6->TabIndex = 29;
			// 
			// groupBox2
			// 
			this->groupBox2->Controls->Add(this->radioButton12);
			this->groupBox2->Controls->Add(this->label19);
			this->groupBox2->Controls->Add(this->label18);
			this->groupBox2->Controls->Add(this->radioButton7);
			this->groupBox2->Controls->Add(this->textBox6);
			this->groupBox2->Controls->Add(this->textBox5);
			this->groupBox2->Controls->Add(this->label9);
			this->groupBox2->Controls->Add(this->label8);
			this->groupBox2->Controls->Add(this->radioButton6);
			this->groupBox2->Controls->Add(this->radioButton5);
			this->groupBox2->Location = System::Drawing::Point(33, 516);
			this->groupBox2->Margin = System::Windows::Forms::Padding(4);
			this->groupBox2->Name = L"groupBox2";
			this->groupBox2->Padding = System::Windows::Forms::Padding(4);
			this->groupBox2->Size = System::Drawing::Size(615, 165);
			this->groupBox2->TabIndex = 30;
			this->groupBox2->TabStop = false;
			this->groupBox2->Text = L"TYPE OF BRANCH SEARCH";
			// 
			// radioButton12
			// 
			this->radioButton12->AutoSize = true;
			this->radioButton12->Location = System::Drawing::Point(49, 59);
			this->radioButton12->Margin = System::Windows::Forms::Padding(4);
			this->radioButton12->Name = L"radioButton12";
			this->radioButton12->Size = System::Drawing::Size(140, 19);
			this->radioButton12->TabIndex = 33;
			this->radioButton12->TabStop = true;
			this->radioButton12->Text = L"Forward+Backward";
			this->radioButton12->UseVisualStyleBackColor = true;
			this->radioButton12->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton12_CheckedChanged);
			// 
			// label19
			// 
			this->label19->AutoSize = true;
			this->label19->Location = System::Drawing::Point(33, 96);
			this->label19->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label19->Name = L"label19";
			this->label19->Size = System::Drawing::Size(87, 15);
			this->label19->TabIndex = 32;
			this->label19->Text = L"Global Search";
			// 
			// label18
			// 
			this->label18->AutoSize = true;
			this->label18->Location = System::Drawing::Point(35, 28);
			this->label18->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label18->Name = L"label18";
			this->label18->Size = System::Drawing::Size(81, 15);
			this->label18->TabIndex = 31;
			this->label18->Text = L"Local Search";
			// 
			// radioButton7
			// 
			this->radioButton7->AutoSize = true;
			this->radioButton7->Location = System::Drawing::Point(305, 59);
			this->radioButton7->Margin = System::Windows::Forms::Padding(4);
			this->radioButton7->Name = L"radioButton7";
			this->radioButton7->Size = System::Drawing::Size(84, 19);
			this->radioButton7->TabIndex = 30;
			this->radioButton7->TabStop = true;
			this->radioButton7->Text = L"Backward";
			this->radioButton7->UseVisualStyleBackColor = true;
			this->radioButton7->CheckedChanged += gcnew System::EventHandler(this, &Form1::radioButton7_CheckedChanged);
			// 
			// textBox2
			// 
			this->textBox2->Location = System::Drawing::Point(175, 149);
			this->textBox2->Margin = System::Windows::Forms::Padding(4);
			this->textBox2->Name = L"textBox2";
			this->textBox2->Size = System::Drawing::Size(557, 25);
			this->textBox2->TabIndex = 31;
			this->textBox2->TextChanged += gcnew System::EventHandler(this, &Form1::textBox2_TextChanged_1);
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(31, 152);
			this->label5->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(125, 15);
			this->label5->TabIndex = 32;
			this->label5->Text = L"Branch Specification";
			// 
			// button6
			// 
			this->button6->Location = System::Drawing::Point(767, 148);
			this->button6->Margin = System::Windows::Forms::Padding(4);
			this->button6->Name = L"button6";
			this->button6->Size = System::Drawing::Size(100, 29);
			this->button6->TabIndex = 33;
			this->button6->Text = L"Browse";
			this->button6->UseVisualStyleBackColor = true;
			this->button6->Click += gcnew System::EventHandler(this, &Form1::button6_Click);
			// 
			// label11
			// 
			this->label11->AutoSize = true;
			this->label11->Location = System::Drawing::Point(681, 209);
			this->label11->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label11->Name = L"label11";
			this->label11->Size = System::Drawing::Size(38, 15);
			this->label11->TabIndex = 34;
			this->label11->Text = L"Year:";
			// 
			// textBox8
			// 
			this->textBox8->Location = System::Drawing::Point(729, 225);
			this->textBox8->Margin = System::Windows::Forms::Padding(4);
			this->textBox8->Name = L"textBox8";
			this->textBox8->Size = System::Drawing::Size(103, 25);
			this->textBox8->TabIndex = 35;
			// 
			// label12
			// 
			this->label12->AutoSize = true;
			this->label12->Location = System::Drawing::Point(841, 229);
			this->label12->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label12->Name = L"label12";
			this->label12->Size = System::Drawing::Size(23, 15);
			this->label12->TabIndex = 36;
			this->label12->Text = L"To";
			// 
			// textBox9
			// 
			this->textBox9->Location = System::Drawing::Point(873, 225);
			this->textBox9->Margin = System::Windows::Forms::Padding(4);
			this->textBox9->Name = L"textBox9";
			this->textBox9->Size = System::Drawing::Size(103, 25);
			this->textBox9->TabIndex = 37;
			// 
			// label17
			// 
			this->label17->AutoSize = true;
			this->label17->Location = System::Drawing::Point(681, 229);
			this->label17->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label17->Name = L"label17";
			this->label17->Size = System::Drawing::Size(38, 15);
			this->label17->TabIndex = 38;
			this->label17->Text = L"From";
			// 
			// label20
			// 
			this->label20->AutoSize = true;
			this->label20->Location = System::Drawing::Point(35, 30);
			this->label20->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label20->Name = L"label20";
			this->label20->Size = System::Drawing::Size(111, 15);
			this->label20->TabIndex = 39;
			this->label20->Text = L"Clan Specification";
			// 
			// textBox12
			// 
			this->textBox12->Location = System::Drawing::Point(165, 26);
			this->textBox12->Margin = System::Windows::Forms::Padding(4);
			this->textBox12->Name = L"textBox12";
			this->textBox12->Size = System::Drawing::Size(535, 25);
			this->textBox12->TabIndex = 40;
			this->textBox12->TextChanged += gcnew System::EventHandler(this, &Form1::textBox12_TextChanged);
			// 
			// groupBox3
			// 
			this->groupBox3->Controls->Add(this->button8);
			this->groupBox3->Controls->Add(this->button7);
			this->groupBox3->Controls->Add(this->label20);
			this->groupBox3->Controls->Add(this->textBox12);
			this->groupBox3->Location = System::Drawing::Point(32, 701);
			this->groupBox3->Margin = System::Windows::Forms::Padding(4);
			this->groupBox3->Name = L"groupBox3";
			this->groupBox3->Padding = System::Windows::Forms::Padding(4);
			this->groupBox3->Size = System::Drawing::Size(1015, 72);
			this->groupBox3->TabIndex = 43;
			this->groupBox3->TabStop = false;
			this->groupBox3->Text = L"CLAN ANALYSIS";
			// 
			// button8
			// 
			this->button8->Location = System::Drawing::Point(756, 26);
			this->button8->Margin = System::Windows::Forms::Padding(4);
			this->button8->Name = L"button8";
			this->button8->Size = System::Drawing::Size(100, 29);
			this->button8->TabIndex = 47;
			this->button8->Text = L"Browse";
			this->button8->UseVisualStyleBackColor = true;
			this->button8->Click += gcnew System::EventHandler(this, &Form1::button8_Click);
			// 
			// button7
			// 
			this->button7->Location = System::Drawing::Point(876, 26);
			this->button7->Margin = System::Windows::Forms::Padding(4);
			this->button7->Name = L"button7";
			this->button7->Size = System::Drawing::Size(100, 29);
			this->button7->TabIndex = 46;
			this->button7->Text = L"Setting";
			this->button7->UseVisualStyleBackColor = true;
			this->button7->Click += gcnew System::EventHandler(this, &Form1::button7_Click);
			// 
			// label21
			// 
			this->label21->AutoSize = true;
			this->label21->Location = System::Drawing::Point(871, 575);
			this->label21->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label21->Name = L"label21";
			this->label21->Size = System::Drawing::Size(82, 15);
			this->label21->TabIndex = 44;
			this->label21->Text = L"Decay Factor";
			// 
			// textBox13
			// 
			this->textBox13->Location = System::Drawing::Point(873, 591);
			this->textBox13->Margin = System::Windows::Forms::Padding(4);
			this->textBox13->Name = L"textBox13";
			this->textBox13->Size = System::Drawing::Size(119, 25);
			this->textBox13->TabIndex = 45;
			this->textBox13->TextChanged += gcnew System::EventHandler(this, &Form1::textBox13_TextChanged_1);
			// 
			// label22
			// 
			this->label22->AutoSize = true;
			this->label22->Location = System::Drawing::Point(671, 268);
			this->label22->Margin = System::Windows::Forms::Padding(4, 0, 4, 0);
			this->label22->Name = L"label22";
			this->label22->Size = System::Drawing::Size(118, 15);
			this->label22->TabIndex = 46;
			this->label22->Text = L"Clustering Network";
			this->label22->Click += gcnew System::EventHandler(this, &Form1::label22_Click);
			// 
			// comboBox4
			// 
			this->comboBox4->FormattingEnabled = true;
			this->comboBox4->Items->AddRange(gcnew cli::array< System::Object^  >(6) {L"No", L"Citation (undirected/unweighted)", L"Coauthor (unweighted)", 
				L"Coword (unweighted)", L"Coassignee (unweighted)", L"Citation (directed/unweighted)"});
			this->comboBox4->Location = System::Drawing::Point(674, 287);
			this->comboBox4->Margin = System::Windows::Forms::Padding(4);
			this->comboBox4->Name = L"comboBox4";
			this->comboBox4->Size = System::Drawing::Size(172, 23);
			this->comboBox4->TabIndex = 47;
			this->comboBox4->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::comboBox4_SelectedIndexChanged);
			// 
			// label24
			// 
			this->label24->AutoSize = true;
			this->label24->Location = System::Drawing::Point(870, 367);
			this->label24->Name = L"label24";
			this->label24->Size = System::Drawing::Size(116, 15);
			this->label24->TabIndex = 51;
			this->label24->Text = L"Relevancy Strategy";
			this->label24->Click += gcnew System::EventHandler(this, &Form1::label24_Click);
			// 
			// comboBox6
			// 
			this->comboBox6->FormattingEnabled = true;
			this->comboBox6->Items->AddRange(gcnew cli::array< System::Object^  >(7) {L"Flat", L"CPC+Jaccard", L"CPC3+Jaccard", L"CPC4+Jaccard", 
				L"Citation+Jaccard", L"CPC3+Citation+Jaccard", L"CPC4+Citation+Jaccard"});
			this->comboBox6->Location = System::Drawing::Point(873, 385);
			this->comboBox6->Name = L"comboBox6";
			this->comboBox6->Size = System::Drawing::Size(172, 23);
			this->comboBox6->TabIndex = 50;
			this->comboBox6->SelectedIndexChanged += gcnew System::EventHandler(this, &Form1::comboBox6_SelectedIndexChanged);
			// 
			// Form1
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 15);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->AutoSize = true;
			this->ClientSize = System::Drawing::Size(1057, 702);
			this->Controls->Add(this->label24);
			this->Controls->Add(this->comboBox6);
			this->Controls->Add(this->comboBox4);
			this->Controls->Add(this->label22);
			this->Controls->Add(this->textBox13);
			this->Controls->Add(this->label21);
			this->Controls->Add(this->label17);
			this->Controls->Add(this->textBox9);
			this->Controls->Add(this->label12);
			this->Controls->Add(this->textBox8);
			this->Controls->Add(this->label11);
			this->Controls->Add(this->button6);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->textBox2);
			this->Controls->Add(this->groupBox2);
			this->Controls->Add(this->button5);
			this->Controls->Add(this->textBox4);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->groupBox1);
			this->Controls->Add(this->comboBox3);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->comboBox2);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->comboBox1);
			this->Controls->Add(this->button4);
			this->Controls->Add(this->button3);
			this->Controls->Add(this->button2);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->groupBox3);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->Margin = System::Windows::Forms::Padding(4);
			this->Name = L"Form1";
			this->Text = L"MainPath Professional: A main path search program";
			this->Load += gcnew System::EventHandler(this, &Form1::Form1_Load);
			this->groupBox1->ResumeLayout(false);
			this->groupBox1->PerformLayout();
			this->groupBox2->ResumeLayout(false);
			this->groupBox2->PerformLayout();
			this->groupBox3->ResumeLayout(false);
			this->groupBox3->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
			 if(openFileDialog1->ShowDialog()==System::Windows::Forms::DialogResult::OK) {
				 textBox4->Text=openFileDialog1->FileName;
			 }
			 }
private: System::Void button4_Click(System::Object^  sender, System::EventArgs^  e) {
			 Application::Exit();
		 }
private: System::Void button2_Click(System::Object^  sender, System::EventArgs^  e) {
			 // this section check if the software is legal (this is done also at loading)
			 wchar_t licensee_name[BLOCK_SIZE];	// for user name
			 wchar_t licensee_org[BLOCK_SIZE];		// for user organization
			 char strSerial[BLOCK_SIZE];		// for serial string
			 string strMachineCode;	// for machine code
			 String^ mymsg = "";
			 String^ license_notice1 = "";
			 if (version_id == 3 || version_id == 2 || version_id == 4 || version_id == 5)	// partner, student, law, and light versions are legal
			 {
				 legal = TRUE;
				 if (version_id == 2 && release_id == 1)
					 license_notice1 = String::Concat("Limited Version\nExpiration Date: 2018/07/31\n\n");;
				 if (version_id == 5 && release_id == 1)	// LIGHT version
					 license_notice1 = String::Concat("Limited Version\nExpiration Date: 2018/07/31\n\n");
				 if (version_id == 2 && release_id == 2)
					 license_notice1 = String::Concat("Limited Pro Version\nExpiration Date: 2018/07/31\n\n");
			 }
			 else					// do legality check for non-student version
			 {
				 strMachineCode = GetMachineCode();
				 ReadSerial(&licensee_name[0], &licensee_org[0], &strSerial[0]);
				 legal = CheckLicense(strSerial);
				 // end software legality check
				 int i;
				 if (legal)		
				 {
					license_notice1 = String::Concat("This software is licensed to ");
					for (i = 0; i < (int)wcslen(licensee_name); i++) 
						license_notice1 = String::Concat(license_notice1, Convert::ToString(licensee_name[i]));
					license_notice1 = String::Concat(license_notice1, " of ");
					for (i = 0; i < (int)wcslen(licensee_org); i++) 
						license_notice1 = String::Concat(license_notice1, Convert::ToString(licensee_org[i]));
					license_notice1 = String::Concat(license_notice1, ".\n\n");
				 }
				 else
					license_notice1 = String::Concat("!!! ILLEGAL SOFTWARE KEY !!!\n\n");
			}
			 String^ description1 = "This program does the following actions:\n1. read given data and/or relationship-list file\n2. establish a citation network\n3. find the main paths and/or branch paths of the given network             \n\n";
			 String^ mymsg1 = "Written By John S. Liu,\nGraduate Institute of Technology Management,\nNational Taiwan University of Science and Technology\n";
			 String^ eaddress1 = "email: johnliu@mail.ntust.edu.tw";
			 String^ description2 = "This program does the following actions:\n1. read a full record file and/or relationship-list file\n2. establish a citation network\n3. find the main paths and branch paths of the given network             \n\n";
			 String^ mymsg2 = "\n";
			 String^ eaddress2 = "";
			 if (release_id == 2)	// for DELTA release, less messages
				mymsg = String::Concat(license_notice1, description2, mymsg2, eaddress2);
			 else
				mymsg = String::Concat(license_notice1, description1, mymsg1, eaddress1);
			 MessageBox::Show(mymsg,"Information",MessageBoxButtons::OK,MessageBoxIcon::Information);
		 }
private: System::Void button3_Click(System::Object^  sender, System::EventArgs^  e) {
			 // check if the secret unlock code is provided, added 2016/10/08
			 int i; wchar_t iname[FNAME_SIZE];
			 for (i = 0; i < textBox1->Text->Length; i++) 
				iname[i] = textBox1->Text[i];
			 iname[i] = '\0';
			 if (wcscmp(iname, L"*") == 0)
			 {
				UnlockLicense = 2;
				textBox1->Text = L"";
				textBox1->PasswordChar = '*';	// set to password display mode
				return;
			 }
			 else if (UnlockLicense == 2 && wcscmp(iname, L"Da Vincier Lab") == 0)
			 {
				legal = TRUE;
				UnlockLicense = TRUE;
				textBox1->PasswordChar = '\0';	// reset the password '*' display mode, back to normal
				textBox1->Text = L"MainPath license unlocked!";
				return;
			 }
			 if (!legal) exit(0);	// simply exit if the software is not legal
			 if (textBox1->Text->Length==0)
				 return;
			 if (Convert::ToDouble(textBox13->Text)<=0.0 || Convert::ToDouble(textBox13->Text)>1.0)	// illegal values for decay factor
			 {
				 textBox13->Text="0.2";
				 return;
			 }
			 String^ mymsg = "";
			 String^ mymsg1 = "";
			if (version_id == 5)	// for "Light" release, citation information is required to begin the processing
			{
				if (textBox4->Text->Length==0)	// citation file is not provided
				{
					mymsg = String::Concat("Please provide a file that contains relationship list.");
					MessageBox::Show(mymsg);
					return;
				}
			}
			int ind_dot; 
			wchar_t citationname[FNAME_SIZE]; wchar_t outputname[FNAME_SIZE]; wchar_t wosname[FNAME_SIZE]; wchar_t branchname[FNAME_SIZE]; 
			wchar_t clanfilename[FNAME_SIZE];
			wchar_t sversion[200];
			for (i = 0; i < this->Text->Length; i++) 
				sversion[i] = this->Text[i];
			 sversion[i] = '\0';
			for (i = 0; i < textBox2->Text->Length; i++) 
				branchname[i] = textBox2->Text[i];
			 branchname[i] = '\0';
			for (i = 0; i < textBox1->Text->Length; i++) 
				wosname[i] = textBox1->Text[i];
			 wosname[i] = '\0';
			 ind_dot = -1;
			 for (i = 0; i < textBox4->Text->Length; i++) {
				 citationname[i] = textBox4->Text[i];
				 if (citationname[i] == '.')
					 ind_dot = i;	// indicate the index of the last dot in the file name
			 }
			 citationname[i] = '\0';
			 if (ind_dot <= 0)	// no '.' in the input file name
				 ind_dot = textBox4->Text->Length;
			 for (i = 0; i < ind_dot; i++) 
				 outputname[i] = textBox4->Text[i];
			 outputname[i] = '\0';
			 for (i = 0; i < textBox12->Text->Length; i++) 
				clanfilename[i] = textBox12->Text[i];
			 clanfilename[i] = '\0';
			 //wcscat_s(outputname, FNAME_SIZE, L".paj");
			// the codes for FormLaw is added 2012/05/06
			 FILE *sstream;
			 wchar_t line[4096]; 
			 FormLaw ^formLaw = gcnew FormLaw;
			 if (_wfopen_s(&sstream, wosname, L"rt, ccs=UTF-8") != 0)	// open the Westlaw identification file (given as full record file), modified to read as UTF file, 2016/07/09
			 {
				mymsg = String::Concat("Error: File \"", textBox1->Text, "\" not found.");
				MessageBox::Show(mymsg);					
				Application::Exit();
			 }
			 if(fgetws(line, 4096, sstream) == NULL)
			 {
				mymsg = String::Concat("Error: Unknown data.");
				MessageBox::Show(mymsg);					
				Application::Exit();
			 }
			 if (wcsncmp(line, L"WESTLAW DATA", 12) == 0)	// Leading string for Westlaw data, noting that "WESTLAW DATA2" will also pass the check
			 {
				 if (textBox4->Text->Length == 0)	// citation file is not provided
					 formLaw->CitationFileProvided = false;
				 else
					 formLaw->CitationFileProvided = true;
				 formLaw->ShowDialog(this);	// open FormLaw to select Westlaw data options
				 delete formLaw;
			 }
			 fclose(sstream);

			 int ret=main_function(citationname, outputname, wosname, branchname, Separator, GroupFinderOptions, ClusteringCoauthorNetworkOptions, RelevancyStrategy,
				 M1_MODE, MDIRECTED, Direction, SPMethod, SCOnly, MPType, 
				 Convert::ToDouble(textBox3->Text), Convert::ToDouble(textBox7->Text), BPType, Convert::ToDouble(textBox5->Text), 
				 Convert::ToDouble(textBox6->Text), Convert::ToInt32(textBox8->Text), Convert::ToInt32(textBox9->Text), 
				 Convert::ToInt32(textBox10->Text), Convert::ToInt32(textBox11->Text), Convert::ToDouble(textBox13->Text), clanfilename, 
				 formLaw->POSexamined, formLaw->POSdiscussed, formLaw->POScited, formLaw->POSmentioned, 
				 formLaw->NEG4star, formLaw->NEG3star, formLaw->NEG2star, formLaw->NEG1star, formLaw->TakeWeight, sversion, UnlockLicense);
			 for (i = 0; i < (int)wcslen(outputname); i++) {
				 mymsg1 = String::Concat(mymsg1, Convert::ToString(outputname[i]));
			}
			switch(ret) {
				case MSG_FILE_CREATED:
					mymsg = String::Concat("Main paths created successfully.");
					MessageBox::Show(mymsg);
					break;
				case MSG_IFILE_NOTFOUND:
					mymsg = String::Concat("Error: File \"", textBox4->Text, "\" not found.");
					MessageBox::Show(mymsg);
					break;
				case MSG_OFILE_CANNOTOPEN:
					mymsg = String::Concat("Error: Can not create file \"", mymsg1, "\".");
					MessageBox::Show(mymsg);
					break;
				case MSG_OFILE_CANNOTWRITE:
					mymsg = String::Concat("Error: Can not write to file \"", mymsg1, "\".");
					MessageBox::Show(mymsg);
					break;
				case MSG_ACYCLIC:
					mymsg = String::Concat("Error: The network is not acyclic.");
					MessageBox::Show(mymsg);
					break;
				case MSG_WOSFILE_NOTFOUND:
					mymsg = String::Concat("Error: File \"", textBox1->Text, "\" not found.");
					MessageBox::Show(mymsg);
					break;
				case MSG_WOSFILE_FORMAT_ERROR:
					mymsg = String::Concat("Error: Format error in the input data file.");
					MessageBox::Show(mymsg);
					break;				
				case MSG_MEMORY:
					mymsg = String::Concat("Error: Not enough memory, memory allocation error.");
					MessageBox::Show(mymsg);
					break;		
				case MSG_CFILE_CANNOTOPEN:
					mymsg = String::Concat("Error: Can not open patent citation file.");
					MessageBox::Show(mymsg);
					break;		
				case MSG_FILE_CANNOTOPEN:
					mymsg = String::Concat("Error: Can not open file.");
					MessageBox::Show(mymsg);
					break;		
				case MSG_FILE_CITATION_FILE_NOT_PROVIDED:
					mymsg = String::Concat("Error: Please provide a relationship-list file.");
					MessageBox::Show(mymsg);
					break;	
				case MSG_FILE_NAME_TOO_LONG:
					mymsg = String::Concat("Error: File name too long.");
					MessageBox::Show(mymsg);
					break;
				case MSG_CLANFILE_CANNOTOPEN:
					mymsg = String::Concat("Error: Can not open file \"", textBox12->Text, "\".");
					MessageBox::Show(mymsg);
					break;
				case MSG_CLANFILE_FORMAT_ERROR:
					mymsg = String::Concat("Error: Format error in the clan specification file.");
					MessageBox::Show(mymsg);
					break;	
				case MSG_NOT_ENOUGH_MEMORY:
					mymsg = String::Concat("Error: Not enough memory.");
					MessageBox::Show(mymsg);
					break;	
				case MSG_FILE_FORMAT_ERROR:
					mymsg = String::Concat("Error: Unrecognized file format.");
					MessageBox::Show(mymsg);
					break;	
				case MSG_SOFTWARE_EXPIRED:
					mymsg = String::Concat("Error: License expired.");
					MessageBox::Show(mymsg);
					break;		
				case MSG_EXCEEDING_LIMIT:
					mymsg = String::Concat("Error: Exceeding the number-of-records limitation.");
					MessageBox::Show(mymsg);
					break;	 
				case MSG_WESTLAW_FILE_NOTFOUND:
					mymsg = String::Concat("Error: Cannot find WestLaw files.");
					MessageBox::Show(mymsg);
					break;	 
				case MSG_SPX_OVERFLOW:
					mymsg = String::Concat("Error: SPX value overflow.");
					MessageBox::Show(mymsg);
					break;
				case MSG_PARALLEL_DATA_ERROR:
					mymsg = String::Concat("Error: Cannot read parallel data.");
					MessageBox::Show(mymsg);
					break;	 
				default:
					MessageBox::Show("");
			}
			Application::Exit();
		 }
private: System::Void comboBox1_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 Separator=comboBox1->SelectedIndex;
		 }
private: System::Void Form1_Load(System::Object^  sender, System::EventArgs^  e) {
			 GroupFinderOptions = GF_DISABLE;
			 ClusteringCoauthorNetworkOptions = CCoauthor_DISABLE;
			 // this section check if the software is legal (this is done also when info button is pressed)
			 wchar_t licensee_name[BLOCK_SIZE];	// for user name
			 wchar_t licensee_org[BLOCK_SIZE];		// for user organization
			 char strSerial[BLOCK_SIZE];		// for serial string
			 string strMachineCode;	// for machine code
			 release_id = RELEASE_ID;
			 version_id = VERSION_ID;
			 UnlockLicense = 0;
			 if (version_id == 2 || version_id == 3 || version_id == 4 || version_id == 5)	// partner, student, law, and light versions are legal
				 legal = TRUE;
			 else					// do legality check for non-student and non-partner version
			 {
				 strMachineCode = GetMachineCode();
				 ReadSerial(&licensee_name[0], &licensee_org[0], &strSerial[0]);
				 legal = CheckLicense(strSerial);
			 }
			 // end software legality checkfile
			 SYSTEMTIME st;
			 GetSystemTime(&st);
			 String^ pname1n = L"MainPath Partners: A main path search program";
			 String^ pname1s = L"MainPath: A main path search program";
			 String^ pname1t = L"MainPath Light: A main path search program";
			 String^ pname1sp = L"MainPath Pro: A main path search program";
			 String^ pname1p = L"MainPath Professional: A main path search program";
			 String^ pname1l = L"MainPath Law: A main path search program";
			 String^ pname2 = L"NetExplorer: A main path search program";
			 String^ version = "(release 403, 2018/08/09)";
			 if (release_id == 2 && version_id != 2)	// DELTA release and not student-pro version
					this->Text = String::Concat(pname2, L" ", version);
			 else
			 {
				 if (version_id == 1)
					this->Text = String::Concat(pname1p, L" ", version);
				 else if (version_id == 3)
					this->Text = String::Concat(pname1n, L" ", version);
				 else if (version_id == 4)
					this->Text = String::Concat(pname1l, L" ", version);
				 else if (version_id == 5)
					this->Text = String::Concat(pname1t, L" ", version);
				 else
				 {
				    if (release_id == 2)	// student-pro version
						this->Text = String::Concat(pname1sp, L" ", version);
					else
						this->Text = String::Concat(pname1s, L" ", version);
				 }
			 }
			 comboBox1->SelectedIndex=2;
			 comboBox2->SelectedIndex=1;  // 0=SPC, 1=SPLC, 3=SPAD, 4=SPGD, 5=SPHD
			 comboBox3->SelectedIndex=1;
			 comboBox4->SelectedIndex=GF_DISABLE;	// GroupFinder disabled
			 //comboBox5->SelectedIndex=CCoauthor_DISABLE;	// Clustering Coauthor Network disabled
			 comboBox6->SelectedIndex=RELEVANCY_FLAT;	// default sets to flat
			 comboBox6->Enabled=false;
			 label24->Enabled=false;
			 radioButton1->Checked=false; radioButton2->Checked=true; radioButton8->Checked=false;
			 radioButton5->Checked=false; radioButton6->Checked=true; radioButton7->Checked=false;
			 textBox1->Text = L"";		textBox1->Enabled=true;
			 textBox4->Text = L"";		textBox4->Enabled=true;
			 textBox2->Text = L"";		textBox2->Enabled=true;
			 textBox3->Text = L"0.0";	textBox3->Enabled=false;	// tie tolerance (main path, local)
			 textBox7->Text = L"1";		textBox7->Enabled=true;		// number of paths (main path, global)
			 textBox5->Text = L"0.00";	textBox5->Enabled=false;	// tie tolerance (branch, local)
			 textBox6->Text = L"1";	textBox6->Enabled=true;		// number of paths (branch, global)
			 textBox8->Text = L"1800";	// year, from
			 textBox9->Text = Convert::ToString(st.wYear+1);	// year, to; changed to current year plus one, 2014/1011
			 textBox10->Text = L"10";	// number of significant routes (local)
			 textBox11->Text = L"10";	// number of significant routes (global)
			 textBox12->Text = L"";		// clan specification file
			 textBox13->Text = L"0.2";	// default decay factor for SPAD
			 groupBox2->Enabled=false;	
			 groupBox3->Enabled=true;	// groupBox3 (clan analysis) was valid only for DELTA relrease, but it was enable 2017/03/02 for all releases
			 
#ifdef OBSOLETE	// clan analysis is enabled for all releases, 2017/03/02
			 // get the Windows version, Windows 7 => 6.1, Windows 8 ==> 6.2
			 OSVERSIONINFO osvi;
			 BOOL bIsWindows8orLater;
			 ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			 osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			 GetVersionEx(&osvi);
			 bIsWindows8orLater = ((osvi.dwMajorVersion >= 6) && (osvi.dwMinorVersion >= 2));
			 if (release_id != 2)	// not DELTA release, the form is smaller (no clan analysis functions)
			 {
				this->Controls->Remove(this->groupBox3);   
				if(bIsWindows8orLater)
					this->ClientSize = System::Drawing::Size(795, 580); // 2015/04/23, when the system upgraded to Windows 8.1
					//this->ClientSize = System::Drawing::Size(1078, 720);// for unknown reason, pixel size is different in Windows 8.0
				else
					this->ClientSize = System::Drawing::Size(795, 580);	// smaller form frame if there is no Clan Analysis
			 }
#endif OBSOLETE
			 MPType=P_GLOBAL;
			 BPType=P_GLOBAL;
			 Direction=MFORWARD;
			 if (comboBox2->SelectedIndex==3 || comboBox2->SelectedIndex==4)	// SPAD or SPGD
			 {
				 label21->Enabled=true;
				 textBox13->Enabled=true;
			 }
			 else
			 {
				 label21->Enabled=false;
				 textBox13->Enabled=false;
			 }
		 }
private: System::Void comboBox2_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 SPMethod=comboBox2->SelectedIndex;
			 if (SPMethod==S_SPAD || SPMethod==S_SPGD)
			 {
				 label21->Enabled=true;
				 textBox13->Enabled=true;
			 }
			 else
			 {
				 label21->Enabled=false;
				 textBox13->Enabled=false;
			 }
		 }
private: System::Void comboBox3_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 SCOnly=comboBox3->SelectedIndex;
		 }
private: System::Void textBox2_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void radioButton1_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton1->Checked) {
				 textBox3->Enabled=true;	 textBox10->Enabled=false;	textBox7->Enabled=false;	textBox11->Enabled=false;
				 MPType=P_LOCAL_F;
				 Direction=MFORWARD;
				 }
		 }
private: System::Void radioButton2_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton2->Checked) {
				 textBox3->Enabled=false;	 textBox10->Enabled=false;	textBox7->Enabled=true;		textBox11->Enabled=false;
				 MPType=P_GLOBAL;
				 }
		 }
private: System::Void button5_Click(System::Object^  sender, System::EventArgs^  e) {
			 if(openFileDialog1->ShowDialog()==System::Windows::Forms::DialogResult::OK) {
				 textBox1->Text=openFileDialog1->FileName;
				 int i; wchar_t iname[FNAME_SIZE];
				 for (i = 0; i < textBox1->Text->Length; i++) 
					iname[i] = textBox1->Text[i];
				 iname[i] = '\0';
				 FType = WOS_or_Others(iname);	// check the type of the input file 
				groupBox3->Enabled=true;	// clan analysis is enabled for all releases, 2017/03/02
#ifdef OBSOLETE
				 groupBox3->Enabled=false;	
				 if (FType == WOS_DATA && FType == SCOPUS_DATA && release_id == 2) // groupBox3 (clan analysis) is valid only for DELTA relrease
				 {
						groupBox3->Enabled=true;	
				 }
				 else if ((FType == USPTO_DATA || FType == THOMSON_INNOVATION_DATA || FType == WEBPAT2_DATA || FType == WEBPAT3_DATA || 
					 FType == PGUIDER_DATA) && release_id == 2)
				 {
						groupBox3->Enabled=true;
				 }
#endif OBSOLETE
				 if (FType == USPTO_DATA ||  FType == THOMSON_INNOVATION_DATA || FType == WEBPAT2_DATA ||  FType == WEBPAT3_DATA || 
					 FType == PGUIDER_DATA)
				 {
						comboBox6->Items->Clear();
						comboBox6->Items->AddRange(gcnew cli::array< System::Object^  >(7) {L"Flat", L"CPC+Jaccard", L"CPC3+Jaccard", L"CPC4+Jaccard", 
													L"Citation+Jaccard", L"CPC3+Citation+Jaccard", L"CPC4+Citation+Jaccard"});
						comboBox6->Enabled=true;	// relevancy strategy
						label24->Enabled=true;		// relevancy strategy
				 }
				 else if (FType == SCOPUS_DATA || FType == WOS_DATA)
				 {
						comboBox6->Items->Clear();
						comboBox6->Items->AddRange(gcnew cli::array< System::Object^  >(4) {L"Flat", L"Keyword+Jaccard", L"Citation+Jaccard",
													L"Kwd+Citation+Jaccard"});
						comboBox6->Enabled=true;	// relevancy strategy
						label24->Enabled=true;		// relevancy strategy
				 }
				 else
				 {
						comboBox6->Enabled=false;	// relevancy strategy
						label24->Enabled=false;		// relevancy strategy
				 }
			 }
		 }
private: System::Void radioButton5_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton5->Checked) {
				 textBox5->Enabled=true;	 textBox6->Enabled=false;		
				 BPType=P_LOCAL_F;
				 }
		 }
private: System::Void textBox7_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void radioButton6_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton6->Checked) {
				 textBox5->Enabled=false;	 textBox6->Enabled=true;
				 BPType=P_GLOBAL;
				 }
		 }
private: System::Void radioButton7_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton7->Checked) {
				 textBox5->Enabled=true;	 textBox6->Enabled=false;
				 BPType=P_LOCAL_B;
			 }
		 }
private: System::Void radioButton8_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton8->Checked) {
				 textBox3->Enabled=true;	 textBox10->Enabled=false;	textBox7->Enabled=false;		textBox11->Enabled=false;
				 MPType=P_LOCAL_B;
				 Direction=MFORWARD;
				 }
		 }
private: System::Void button6_Click(System::Object^  sender, System::EventArgs^  e) {
			 if(openFileDialog1->ShowDialog()==System::Windows::Forms::DialogResult::OK) {
				 textBox2->Text=openFileDialog1->FileName;
			 }
		 }
private: System::Void textBox2_TextChanged_1(System::Object^  sender, System::EventArgs^  e) {
			if (textBox2->Text->Length==0)
				groupBox2->Enabled=false;
			else
				groupBox2->Enabled=true;
		 }
private: System::Void radioButton3_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {			 
				 textBox3->Enabled=true;	 textBox10->Enabled=true;	textBox7->Enabled=false;	textBox11->Enabled=false;
				 MPType=P_LOCAL_KEY_ROUTE;
				 Direction=MFORWARD;
		 }
private: System::Void radioButton4_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {	 
				 textBox3->Enabled=false;	 textBox10->Enabled=false;	textBox7->Enabled=true;		textBox11->Enabled=true;
				 MPType=P_GLOBAL_KEY_ROUTE;
				 Direction=MFORWARD;
		 }
private: System::Void textBox11_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void textBox13_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void radioButton12_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(radioButton12->Checked) {
				 textBox5->Enabled=true;	 textBox6->Enabled=false;		
				 BPType=P_LOCAL_FB;
				 }
		 }
private: System::Void button7_Click(System::Object^  sender, System::EventArgs^  e) {
			 Form2 ^form2 = gcnew Form2;
			 form2->clanspec = textBox12->Text;
			 form2->ftype = FType;
			 //if (form2->ShowDialog(this) == ::DialogResult::OK)
			 form2->ShowDialog(this);	
			 if (form2->exit_type == 1)	// exit and save to file
			 {
				 textBox12->Text = form2->clanspec;
			 }
			 delete form2;
		 }
private: System::Void textBox12_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void button8_Click(System::Object^  sender, System::EventArgs^  e) {
			 if (openFileDialog1->ShowDialog()==System::Windows::Forms::DialogResult::OK) {
				 textBox12->Text=openFileDialog1->FileName;
		 }
		 }
private: System::Void textBox1_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void textBox4_TextChanged(System::Object^  sender, System::EventArgs^  e) {
			 //if (textBox4->Text->Length == 0)
			 //	groupBox3->Enabled=true;
			 // else
			 //	groupBox3->Enabled=false;
		 }
private: System::Void textBox13_TextChanged_1(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void comboBox4_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 GroupFinderOptions=comboBox4->SelectedIndex;	// added 2014/04/11
			 if (GroupFinderOptions == CCoauthor_UNWEIGHTED)	// added 2016/07/05
				ClusteringCoauthorNetworkOptions = CCoauthor_UNWEIGHTED;
			 else
				ClusteringCoauthorNetworkOptions = CCoauthor_DISABLE;
		 }
private: System::Void label22_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
//private: System::Void comboBox5_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
//			 ClusteringCoauthorNetworkOptions=comboBox5->SelectedIndex;	// added 2015/10/19
//		 }
private: System::Void label23_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void label24_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void comboBox6_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			 RelevancyStrategy=comboBox6->SelectedIndex;	// added 2016/05/07
		 }
};
}

