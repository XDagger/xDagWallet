using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Rpc.Models.Responses
{
    public class ErrorResponse : ModelBase
    {
        [JsonProperty(PropertyName = "message")]
        public string Message
        {
            get; set;
        }

        [JsonProperty(PropertyName = "code")]
        public int Code
        {
            get; set;
        }
    }
}
