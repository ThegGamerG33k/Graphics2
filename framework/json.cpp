//~ #include "json.h"

//~ Json Json::parse(const std::string& data)
//~ {
    //~ json11::Json JJ = json11::parse(data);
    //~ return Json(JJ);
//~ }

//~ Json::Json(json11::Json& JJ)
//~ {
    //~ this->J = JJ;
//~ }

//~ class Json{
  //~ public:
    //~ typedef std::variant< double,nullptr,bool,std::string,std::vector<Json>,std::map<std::string,Json> > JsonData;

    //~ static Json parse(const std::string& data);

    //~ JsonData& operator[](const std::string key);
    //~ const JsonData& operator[](const std::string key) const;


  //~ private:
    //~ json11::Json J;
    //~ Json(const json11::Json& JJ);
//~ };
