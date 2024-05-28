#include "ConfigIni.hpp"

ConfigIni::ConfigIni(string fileName)
{
    //std::locale::global(std::locale("C"));
    m_file_name = fileName;
	Init();
}



bool ConfigIni::Init()
{
    boost::filesystem::path inipath(m_file_name);
    boost::nowide::ifstream ifs(m_file_name);

    if (!boost::filesystem::exists(inipath))
	{
		cout << "file not exists!" << endl;
		return false;
	}
    boost::property_tree::ini_parser::read_ini(ifs, m_root_node);
	return true;

}


//bool ConfigIni::WriteItem(string strRoot_child_name, string value)
//{
//	 put rootnode.childnode value
//	boost::property_tree::ptree root_node = "sys";
//		 put第一个参数需要传入子节点和子节点下元素的名称，中间用.隔开，比如System.pwd
//	this->m_root_node.put<string>(strRoot_child_name, value);
//	write_ini(m_file_name, m_root_node);
//	return true;
//
//}
//template <typename T>
//T ConfigIni::Get_int_value(string node_name, string child_name, T& devalue)
//{
//	boost::property_tree::ptree child_node;
//
//	// return child node
//	child_node = m_root_node.get_child(node_name);
//	
//	// get child value
//	return child_node.get<T>(child_name,devalue);
//	//return true;
//}
//
//bool ConfigIni::UpdateItem(string strRoot_child_name, string value)
//{
//	// put rootnode.childnode value
//	this->m_root_node.put<string>(strRoot_child_name, value);
//
//	write_ini(m_file_name, m_root_node);
//	return true;
//}

