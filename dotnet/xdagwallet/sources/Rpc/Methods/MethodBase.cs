using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.Rpc.Models;

namespace XDagNetWallet.Rpc.Methods
{
    public class MethodBase
    {
        [JsonProperty(PropertyName = "method")]
        public string Method
        {
            get; set;
        }

        [JsonProperty(PropertyName = "id")]
        public int Identifier
        {
            get; set;
        }

        [JsonProperty(PropertyName = "params")]
        public List<ModelBase> Parameters
        {
            get; set;
        }

        public MethodBase(int identifier, string methodName)
        {
            this.Identifier = identifier;
            this.Method = methodName;

            this.Parameters = new List<ModelBase>();
        }

        public string ToJsonString()
        {
            return JsonConvert.SerializeObject(this, Formatting.None);
        }
    }
}
