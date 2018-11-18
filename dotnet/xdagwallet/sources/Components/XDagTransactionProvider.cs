using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.ExplorerAPI;
using XDagNetWallet.ExplorerAPI.Models;

namespace XDagNetWallet.Components
{
    public class XDagTransactionProvider
    {
        public static List<XDagTransaction> GetTransactionHistory(string walletAddress)
        {
            if (string.IsNullOrEmpty(walletAddress))
            {
                return null;
            }

            XDagExplorerClient client = new XDagExplorerClient();
            GetBlockResponse response = client.GetBlock(walletAddress);

            if (response == null || response.TransactionsList == null)
            {
                return null;
            }

            List<XDagTransaction> transactionList = new List<XDagTransaction>();
            foreach(TransactionData data in response.TransactionsList)
            {
                XDagTransaction transaction = new XDagTransaction();

                double amountValue = 0;
                if (double.TryParse(data.Amount, out amountValue))
                {
                    transaction.Amount = amountValue;
                }
                
                transaction.BlockAddress = data.Address;

                DateTime dateTime = DateTime.MinValue;
                if (DateTime.TryParse(data.TimeStamp, out dateTime))
                {
                    transaction.TimeStamp = dateTime;
                }

                transaction.SetDirection(data.Direction);

                /*
                GetBlockResponse tResponse = client.GetBlock(transaction.BlockAddress);
                transaction.SetStatus(tResponse.State);
                transaction.PartnerAddress = GetPartnerAddress(walletAddress, tResponse, data.Direction);
                */

                transactionList.Add(transaction);
            }

            return transactionList;
        }

        /// <summary>
        /// Fill the following fields for a transaction block: 
        ///     1. Status
        ///     2. PartnerAddress
        /// </summary>
        /// <param name="transaction"></param>
        public static void FillTransactionData(string walletAddress, XDagTransaction transaction)
        {
            XDagExplorerClient client = new XDagExplorerClient();
            GetBlockResponse tResponse = client.GetBlock(transaction.BlockAddress);

            transaction.SetStatus(tResponse.State);
            transaction.PartnerAddress = GetPartnerAddress(walletAddress, tResponse, transaction.Direction.ToString());
        }

        private static string GetPartnerAddress(string walletAddress, GetBlockResponse response, string direction)
        {
            foreach(TransactionData data in response.PartnersList)
            {
                if (data.Direction.Equals(direction, StringComparison.InvariantCultureIgnoreCase) && !data.Address.Equals(walletAddress))
                {
                    return data.Address;
                }
            }

            return string.Empty;
        }
    }
}
