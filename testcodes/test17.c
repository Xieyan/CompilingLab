struct Complex
{
	float real ,image;
	struct Attribute attr;
};

int abs(int y)
{
	if (y >= 0) return y;
	else return -y;
}

float abs2(struct Complex x)
{
	struct Complex2 z;
	return x.real * x.real + x.image * x.image;
}

int main()
{
	int i,j,a[10][10];
	struct Attribute
	{
		int mag;
	}k;

	i = 0;
	if (i<=10) j= k.mag = abs(i);

	while (i<10)
	{
		j = j-2;
		i = i+1;
	}	
	
	k && k;
}
