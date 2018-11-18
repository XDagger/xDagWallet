using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.ExplorerAPI.Models
{
    public class ErrorData
    {
        [JsonProperty(PropertyName = "error")]
        public string ErrorCode
        {
            get; set;
        }

        [JsonProperty(PropertyName = "message")]
        public string ErrorMessage
        {
            get; set;
        }

        public ErrorData()
        {

        }
    }
}
