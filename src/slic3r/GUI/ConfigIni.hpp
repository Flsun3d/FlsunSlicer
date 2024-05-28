#pragma once
#include <boost/property_tree/ini_parser.hpp>
#include <boost/exception/all.hpp>
#include <boost/filesystem.hpp> 

#include <wx/wx.h>
using namespace std;


class ConfigIni
{

public:
	ConfigIni(string fileName);
	bool Init();
	//写入配置文件
	template <typename T>
	bool WriteItem(string strRoot_child_name, T& value);
	//遍历节点内
	template <typename T>
	bool GetNodeValue(string node_name, T& devalue);
    void                      DeleteNodeValue()
    {
        boost::nowide::ofstream ifs(m_file_name);
        m_root_node.clear();
        boost::property_tree::ini_parser::write_ini(ifs, m_root_node);
    }
	//获得某个键的值
	template <typename T>
	bool GetValue(std::string key, T& value);

	//更改某个键的值
	template <typename T>
	bool UpdateItem(string strRoot_child_name, T& value);
	boost::property_tree::ptree GetRootNode(){return m_root_node;}


private:
	string m_file_name;
	boost::property_tree::ptree m_root_node;
};

template<typename T>
inline bool ConfigIni::GetValue(std::string key, T& value)
{
	try
	{
		value = this->m_root_node.get<T>(key);
	}
	catch (std::exception e)
	{
		std::cerr << e.what() << std::endl;
		return false;
	}
	return true;
}

template<typename T>
inline bool ConfigIni::UpdateItem(string strRoot_child_name, T& value)
{
		// put rootnode.childnode value
	this->m_root_node.put<T>(strRoot_child_name, value);

    boost::nowide::ofstream ifs(m_file_name);
    write_ini(ifs, m_root_node);
	return true;
}

template<typename T>
inline bool ConfigIni::WriteItem(string strRoot_child_name, T& value)
{
	// put rootnode.childnode value
//boost::property_tree::ptree root_node = "sys";
	// put第一个参数需要传入子节点和子节点下元素的名称，中间用.隔开，比如System.pwd
	this->m_root_node.put<T>(strRoot_child_name, value);
    boost::nowide::ofstream ifs(m_file_name);
	write_ini(ifs, m_root_node);
	return true;
}

template<typename T>
inline bool ConfigIni::GetNodeValue(string node_name, T& devalue)
{
	for (auto& section : m_root_node)
	{
		if(node_name == section.first)
		{ 
			for (auto& key : section.second)
			{
				devalue[key.first] = key.second.get_value<std::string>();

				std::cout << key.first << "=" << key.second.get_value<std::string>() << "\n";

			
			}
			return true;
		}
	}
	return false;
}
