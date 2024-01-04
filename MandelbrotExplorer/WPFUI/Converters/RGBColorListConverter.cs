using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace WPFUI.Converters
{
    public class RGBColorListConverter : IValueConverter
    {
        private RGBColorConverter _rgbconverter = new();

        public uint[] ConvertBack(string colorList)
        {
            uint[] colors = colorList.Split(",").Select(_rgbconverter.ConvertBack).ToArray();
            return colors;
        }

        public string Convert(uint[] colors)
        {
            string colorList = string.Join(", ", colors.Select(_rgbconverter.Convert));
            return colorList;
        }

        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return Convert((uint[])value);
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return ConvertBack(value?.ToString() ?? "");
        }
    }
}
